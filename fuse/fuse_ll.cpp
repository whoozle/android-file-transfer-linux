/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015  Vladimir Menshakov

    Android File Transfer For Linux is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    Android File Transfer For Linux is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Android File Transfer For Linux.
    If not, see <http://www.gnu.org/licenses/>.
 */

#include <fuse_lowlevel.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <mtp/ptp/Device.h>
#include <mtp/ptp/ByteArrayObjectStream.h>
#include <mtp/usb/DeviceNotFoundException.h>
#include <mtp/ptp/ObjectPropertyListParser.h>
#include <mtp/log.h>

#include <map>
#include <string>

namespace
{
	class Exception : public std::runtime_error
	{
	public:
		Exception(const std::string &what) throw() : std::runtime_error(what + ": " + GetErrorMessage(errno)) { }
		Exception(const std::string &what, int returnCode) throw() : std::runtime_error(what + ": " + GetErrorMessage(returnCode)) { }
		static std::string GetErrorMessage(int returnCode)
		{
			char buf[1024];
			std::string text(strerror_r(returnCode, buf, sizeof(buf)));
			return text;
		}
	};

#define FUSE_CALL(...) do { int _r = __VA_ARGS__ ; if (_r < 0) throw Exception(#__VA_ARGS__ " failed", -_r); } while(false)

	typedef std::vector<char> CharArray;

	struct FuseId
	{
		static const FuseId Root;

		fuse_ino_t Inode; //generation here?

		explicit FuseId(fuse_ino_t inode): Inode(inode) { }

		bool operator == (const FuseId &o) const
		{ return Inode == o.Inode; }
		bool operator != (const FuseId &o) const
		{ return !((*this) == o); }
		bool operator < (const FuseId &o) const
		{ return Inode < o.Inode; }
	};

	const FuseId FuseId::Root(FUSE_ROOT_ID);

	struct FuseEntry : fuse_entry_param
	{
		static constexpr const double	Timeout = 60.0;
		static constexpr unsigned 		FileMode 		= S_IFREG | 0444;
		static constexpr unsigned 		DirectoryMode	= S_IFDIR | 0755;

		fuse_req_t Request;

		FuseEntry(fuse_req_t req): fuse_entry_param(), Request(req)
		{
			generation = 1;
			attr_timeout = entry_timeout = Timeout;
		};

		void SetId(FuseId id)
		{
			ino = id.Inode;
			attr.st_ino = id.Inode;
		}

		void SetFileMode()
		{ attr.st_mode = FileMode; }

		void SetDirectoryMode()
		{ attr.st_mode = DirectoryMode; }

		static mode_t GetMode(mtp::ObjectFormat format)
		{
			switch(format)
			{
			case mtp::ObjectFormat::Association:
				return DirectoryMode;
			default:
				return FileMode;
			}
		}

		void Reply()
		{
			FUSE_CALL(fuse_reply_entry(Request, this));
		}

		void ReplyAttr()
		{
			FUSE_CALL(fuse_reply_attr(Request, &attr, Timeout));
		}

		void ReplyError(int err)
		{
			FUSE_CALL(fuse_reply_err(Request, err));
		}
	};

	struct FuseDirectory
	{
		fuse_req_t			Request;

		FuseDirectory(fuse_req_t request): Request(request) { }

		void Add(CharArray & data, const std::string &name, const struct stat & entry)
		{
			if (data.empty())
				data.reserve(4096);
			size_t size = fuse_add_direntry(Request, NULL, 0, name.c_str(), NULL, 0);
			size_t offset = data.size();
			data.resize(data.size() + size);
			fuse_add_direntry(Request, data.data() + offset, size, name.c_str(), &entry, data.size()); //request is not used inside fuse here, so we could cache resulting dirent data
		}

		static void Reply(fuse_req_t req, const CharArray &data, off_t off, size_t size)
		{
			if (off >= (off_t)data.size())
				FUSE_CALL(fuse_reply_buf(req, NULL, 0));
			else
			{
				FUSE_CALL(fuse_reply_buf(req, data.data() + off, std::min<size_t>(size, data.size() - off)));
			}
		}
	};

	class FuseWrapper
	{
		std::mutex		_mutex;
		mtp::DevicePtr	_device;
		mtp::SessionPtr	_session;
		bool			_editObjectSupported;
		bool			_getObjectPropertyListSupported;
		time_t			_connectTime;

		typedef std::map<std::string, FuseId> ChildrenObjects;
		typedef std::map<FuseId, ChildrenObjects> Files;
		Files			_files;

		typedef std::map<mtp::ObjectId, struct stat> ObjectAttrs;
		ObjectAttrs		_objectAttrs;

		typedef mtp::Session::ObjectEditSessionPtr ObjectEditSessionPtr;
		typedef std::map<FuseId, ObjectEditSessionPtr> OpenedFiles;
		OpenedFiles		_openedFiles;

		typedef std::map<FuseId, CharArray> DirectoryCache;
		DirectoryCache	_directoryCache;

		static const size_t					MtpStorageShift = FUSE_ROOT_ID + 1;
		static const size_t					MtpObjectShift = 999998 + MtpStorageShift;

		std::vector<mtp::StorageId>					_storageIdList;
		std::map<mtp::StorageId, std::string>		_storageToName;
		std::map<std::string, mtp::StorageId>		_storageFromName;

	private:
		static FuseId ToFuse(mtp::ObjectId id)
		{ return FuseId(id.Id + MtpObjectShift); }

		static mtp::ObjectId FromFuse(FuseId id)
		{ return mtp::ObjectId(id.Inode - MtpObjectShift); }

		static bool IsStorage(FuseId id)
		{ return id.Inode >= MtpStorageShift && id.Inode <= MtpObjectShift; }

		mtp::StorageId FuseIdToStorageId(FuseId id) const
		{
			if (!IsStorage(id))
				throw std::runtime_error("converting non-storage inode to storage id");
			size_t i = id.Inode - MtpStorageShift;
			if (i >= _storageIdList.size())
				throw std::runtime_error("invalid storage id");
			return _storageIdList[i];
		}

		FuseId FuseIdFromStorageId(mtp::StorageId storageId) const
		{
			auto i = std::find(_storageIdList.begin(), _storageIdList.end(), storageId);
			if (i == _storageIdList.end())
				throw std::runtime_error("invalid storage id");
			return FuseId(MtpStorageShift + std::distance(_storageIdList.begin(), i));
		}

		void GetObjectInfo(ChildrenObjects &cache, mtp::ObjectId id)
		{
			mtp::msg::ObjectInfo oi = _session->GetObjectInfo(id);

			FuseId inode = ToFuse(id);
			cache.emplace(oi.Filename, inode);

			struct stat &attr = _objectAttrs[id];
			attr.st_ino = inode.Inode;
			attr.st_mode = FuseEntry::GetMode(oi.ObjectFormat);
			attr.st_atime = attr.st_mtime = mtp::ConvertDateTime(oi.ModificationDate);
			attr.st_ctime = mtp::ConvertDateTime(oi.CaptureDate);
			attr.st_size = oi.ObjectCompressedSize != mtp::MaxObjectSize? oi.ObjectCompressedSize: _session->GetObjectIntegerProperty(id, mtp::ObjectProperty::ObjectSize);
		}

		struct stat GetObjectAttr(FuseId inode)
		{
			if (inode == FuseId::Root)
			{
				struct stat attr = { };
				attr.st_ino = inode.Inode;
				attr.st_mtime = attr.st_ctime = attr.st_atime = _connectTime;
				attr.st_mode = FuseEntry::DirectoryMode;
				return attr;
			}

			if (IsStorage(inode))
			{
				struct stat attr = { };
				attr.st_ino = inode.Inode;
				attr.st_mtime = attr.st_ctime = attr.st_atime = _connectTime;
				attr.st_mode = FuseEntry::DirectoryMode;
				return attr;
			}

			mtp::ObjectId id = FromFuse(inode);
			auto i = _objectAttrs.find(id);
			if (i != _objectAttrs.end())
				return i->second;

			//populate cache for parent
			auto parent = GetParentObject(inode);
			GetChildren(parent); //populate cache

			i = _objectAttrs.find(id);
			if (i != _objectAttrs.end())
				return i->second;
			else
				throw std::runtime_error("no such object");
		}

		ChildrenObjects & GetChildren(FuseId inode)
		{
			if (inode == FuseId::Root)
			{
				PopulateStorages();
				ChildrenObjects & cache = _files[inode];
				cache.clear();
				for(size_t i = 0; i < _storageIdList.size(); ++i)
				{
					mtp::StorageId storageId = _storageIdList[i];
					auto name = _storageToName.find(storageId);
					if (name != _storageToName.end())
						cache.emplace(name->second, FuseId(MtpStorageShift + i));
					else
						mtp::error("no storage name for ", storageId);
				}
				return cache;
			}

			{
				auto i = _files.find(inode);
				if (i != _files.end())
					return i->second;
			}

			ChildrenObjects & cache = _files[inode];

			using namespace mtp;
			if (inode == FuseId::Root)
			{
				return cache;
			}

			msg::ObjectHandles oh;

			if (IsStorage(inode))
			{
				mtp::StorageId storageId = FuseIdToStorageId(inode);
				oh = _session->GetObjectHandles(storageId, mtp::ObjectFormat::Any, mtp::Session::Root);
			}
			else
			{
				mtp::ObjectId parent = FromFuse(inode);

				if (_getObjectPropertyListSupported)
				{
					//populate filenames
					ByteArray data;
					{
						data = _session->GetObjectPropertyList(parent, ObjectFormat::Any, ObjectProperty::ObjectFilename, 0, 1);
						ObjectPropertyListParser<std::string> parser;
						parser.Parse(data, [&cache](ObjectId objectId, const std::string &name)
						{
							cache.emplace(name, ToFuse(objectId));
						});
					}

					//format
					{
						data = _session->GetObjectPropertyList(parent, ObjectFormat::Any, ObjectProperty::ObjectFormat, 0, 1);
						ObjectPropertyListParser<mtp::ObjectFormat> parser;
						parser.Parse(data, [this](ObjectId objectId, mtp::ObjectFormat format)
						{
							struct stat & attr = _objectAttrs[objectId];
							attr.st_ino = ToFuse(objectId).Inode;
							attr.st_mode = FuseEntry::GetMode(format);
						});
					}

					//size
					{
						data = _session->GetObjectPropertyList(parent, ObjectFormat::Any, ObjectProperty::ObjectSize, 0, 1);
						ObjectPropertyListParser<u64> parser;
						parser.Parse(data, [this](ObjectId objectId, u64 size)
						{
							_objectAttrs[objectId].st_size = size;
						});
					}

					//mtime
					try
					{
						data = _session->GetObjectPropertyList(parent, ObjectFormat::Any, ObjectProperty::DateModified, 0, 1);
						ObjectPropertyListParser<std::string> parser;
						parser.Parse(data, [this](ObjectId objectId, const std::string & mtime)
						{
							time_t t = mtp::ConvertDateTime(mtime);
							_objectAttrs[objectId].st_mtime = t;
						});
					}
					catch(const std::exception &ex)
					{ }

					//ctime
					try
					{
						data = _session->GetObjectPropertyList(parent, ObjectFormat::Any, ObjectProperty::DateAdded, 0, 1);
						ObjectPropertyListParser<std::string> parser;
						parser.Parse(data, [this](ObjectId objectId, const std::string & ctime)
						{
							time_t t = mtp::ConvertDateTime(ctime);
							_objectAttrs[objectId].st_ctime = t;
						});
					}
					catch(const std::exception &ex)
					{ }
					return cache;
				}
				oh = _session->GetObjectHandles(mtp::Session::AllStorages, mtp::ObjectFormat::Any, parent);
			}

			for(auto id : oh.ObjectHandles)
			{
				try
				{
					GetObjectInfo(cache, id);
				} catch(const std::exception &ex)
				{ }
			}
			return cache;
		}

		FuseId CreateObject(FuseId parentInode, const std::string &filename, mtp::ObjectFormat format)
		{
			mtp::ObjectId parentId = FromFuse(parentInode);
			mtp::StorageId storageId;
			if (IsStorage(parentInode))
			{
				storageId = FuseIdToStorageId(parentInode);
				parentId = mtp::Session::Root;
			}
			else
				storageId = _session->GetObjectStorage(parentId);

			mtp::Session::NewObjectInfo noi;
			if (format != mtp::ObjectFormat::Association)
			{
				mtp::msg::ObjectInfo oi;
				oi.Filename = filename;
				oi.ObjectFormat = format;
				noi = _session->SendObjectInfo(oi, storageId, parentId);
				_session->SendObject(std::make_shared<mtp::ByteArrayObjectInputStream>(mtp::ByteArray()));
			}
			else
				noi = _session->CreateDirectory(filename, parentId, storageId);

			mtp::debug("   new object id ", noi.ObjectId.Id);

			{ //update cache:
				auto i = _files.find(parentInode);
				if (i != _files.end())
					GetObjectInfo(i->second, noi.ObjectId); //insert object info into cache
				_directoryCache.erase(parentInode);
			}
			return ToFuse(noi.ObjectId);
		}

		void CreateObject(mtp::ObjectFormat format, fuse_req_t req, FuseId parentId, const char *name, mode_t mode)
		{
			if (parentId == FuseId::Root)
			{
				FUSE_CALL(fuse_reply_err(req, EPERM)); //cannot create files in the same level with storages
				return;
			}
			auto objectId = CreateObject(parentId, name, format);
			FuseEntry entry(req);
			entry.SetId(objectId);
			entry.attr = GetObjectAttr(objectId);
			entry.Reply();
		}

		FuseId GetParentObject(FuseId inode)
		{
			if (inode == FuseId::Root)
				return inode;

			if (IsStorage(inode))
				return FuseId::Root;

			mtp::ObjectId id = FromFuse(inode);
			mtp::ObjectId parent = _session->GetObjectParent(id);
			if (parent == mtp::Session::Device || parent == mtp::Session::Root) //parent == root -> storage
			{
				return FuseIdFromStorageId(_session->GetObjectStorage(id));
			}
			else
				return ToFuse(parent);
		}

		bool FillEntry(FuseEntry &entry, FuseId id)
		{
			try { entry.attr = GetObjectAttr(id); } catch(const std::exception &ex) { return false; }
			entry.SetId(id);
			return true;
		}

	public:
		FuseWrapper()
		{ Connect(); }

		void Connect()
		{
			mtp::scoped_mutex_lock l(_mutex);

			_openedFiles.clear();
			_files.clear();
			_objectAttrs.clear();
			_directoryCache.clear();
			_session.reset();
			_device.reset();
			_device = mtp::Device::Find();
			if (!_device)
				throw std::runtime_error("no MTP device found");

			_session = _device->OpenSession(1);
			_editObjectSupported = _session->EditObjectSupported();
			if (!_editObjectSupported)
				mtp::error("your device does not have android EditObject extension, mounting read-only\n");
			_getObjectPropertyListSupported = _session->GetObjectPropertyListSupported();
			if (!_getObjectPropertyListSupported)
				mtp::error("your device does not have GetObjectPropertyList extension, expect slow enumeration of big directories\n");

			_connectTime = time(NULL);
			PopulateStorages();
		}

		void PopulateStorages()
		{
			_storageIdList.clear();
			_storageFromName.clear();
			_storageToName.clear();
			mtp::msg::StorageIDs ids = _session->GetStorageIDs();
			_storageIdList.reserve(ids.StorageIDs.size());
			for(size_t i = 0; i < ids.StorageIDs.size(); ++i)
			{
				mtp::StorageId id = ids.StorageIDs[i];
				mtp::msg::StorageInfo si = _session->GetStorageInfo(id);
				std::string path = (!si.StorageDescription.empty()? si.StorageDescription:  si.VolumeLabel);
				if (path.empty())
				{
					char buf[64];
					snprintf(buf, sizeof(buf), "sdcard%u", (unsigned)i);
					path = buf;
				}
				FuseId inode(MtpStorageShift + i);
				_storageIdList.push_back(id);
				_storageFromName[path] = id;
				_storageToName[id] = path;
			}
		}

		void Init(void *, fuse_conn_info *conn)
		{
			mtp::scoped_mutex_lock l(_mutex);
			conn->want |= conn->capable & FUSE_CAP_BIG_WRITES; //big writes
			static const size_t MaxWriteSize = 1024 * 1024;
			if (conn->max_write < MaxWriteSize)
				conn->max_write = MaxWriteSize;
		}

		void Lookup (fuse_req_t req, FuseId parent, const char *name)
		{
			mtp::scoped_mutex_lock l(_mutex);
			FuseEntry entry(req);

			const ChildrenObjects & children = GetChildren(parent);
			auto it = children.find(name);
			if (it != children.end())
			{
				if (FillEntry(entry, it->second))
				{
					entry.Reply();
					return;
				}
			}
			entry.ReplyError(ENOENT);
		}

		void ReadDir(fuse_req_t req, FuseId ino, size_t size, off_t off, struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			if (!(GetObjectAttr(ino).st_mode & S_IFDIR))
			{
				FUSE_CALL(fuse_reply_err(req, ENOTDIR));
				return;
			}

			FuseDirectory dir(req);
			auto it = _directoryCache.find(ino);
			if (it == _directoryCache.end())
			{
				const ChildrenObjects & cache = GetChildren(ino);

				it = _directoryCache.insert(std::make_pair(ino, CharArray())).first;
				CharArray &data = it->second;

				dir.Add(data, ".", GetObjectAttr(FuseId::Root));
				dir.Add(data, "..", GetObjectAttr(GetParentObject(ino)));
				for(auto entry : cache)
				{
					dir.Add(data, entry.first, GetObjectAttr(entry.second));
				}
			}

			dir.Reply(req, it->second, off, size);
		}

		void GetAttr(fuse_req_t req, FuseId ino, struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			FuseEntry entry(req);
			if (FillEntry(entry, ino))
				entry.ReplyAttr();
			else
				entry.ReplyError(ENOENT);
		}

		void Read(fuse_req_t req, FuseId ino, size_t size, off_t off, struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			mtp::ByteArray data = _session->GetPartialObject(FromFuse(ino), off, size);
			FUSE_CALL(fuse_reply_buf(req, static_cast<char *>(static_cast<void *>(data.data())), data.size()));
		}

		void Write(fuse_req_t req, FuseId ino, const char *buf, size_t size, off_t off, struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			mtp::Session::ObjectEditSessionPtr tr;
			{
				auto it = _openedFiles.find(ino);
				if (it != _openedFiles.end())
					tr = it->second;
				else
				{
					tr = mtp::Session::EditObject(_session, FromFuse(ino));
					_openedFiles[ino] = tr;
				}
			}
			NOT_NULL(tr)->Send(off, mtp::ByteArray(buf, buf + size));
			FUSE_CALL(fuse_reply_write(req, size));
		}

		void MakeNode(fuse_req_t req, FuseId parent, const char *name, mode_t mode, dev_t rdev)
		{ mtp::scoped_mutex_lock l(_mutex); CreateObject(mtp::ObjectFormat::Undefined, req, parent, name, mode); }

		void MakeDir(fuse_req_t req, FuseId parent, const char *name, mode_t mode)
		{ mtp::scoped_mutex_lock l(_mutex); CreateObject(mtp::ObjectFormat::Association, req, parent, name, mode); }

		void Open(fuse_req_t req, FuseId ino, struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			mtp::ObjectFormat format;

			try
			{
				format = static_cast<mtp::ObjectFormat>(_session->GetObjectIntegerProperty(FromFuse(ino), mtp::ObjectProperty::ObjectFormat));
			}
			catch(const std::exception &ex)
			{ FUSE_CALL(fuse_reply_err(req, ENOENT)); return; }

			if (format == mtp::ObjectFormat::Association)
			{
				FUSE_CALL(fuse_reply_err(req, EISDIR));
				return;
			}
			FUSE_CALL(fuse_reply_open(req, fi));
		}

		void Release(fuse_req_t req, FuseId ino, struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			auto i = _openedFiles.find(ino);
			if (i != _openedFiles.end())
				_openedFiles.erase(i);
		}

		void SetAttr(fuse_req_t req, FuseId ino, struct stat *attr, int to_set, struct fuse_file_info *fi)
		{ GetAttr(req, ino, fi); }

		void RemoveDir (fuse_req_t req, FuseId parent, const char *name)
		{ Unlink(req, parent, name); }

		void Unlink(fuse_req_t req, FuseId parent, const char *name)
		{
			mtp::scoped_mutex_lock l(_mutex);
			ChildrenObjects &children = GetChildren(parent);
			auto i = children.find(name);
			if (i == children.end())
			{
				FUSE_CALL(fuse_reply_err(req, ENOENT));
				return;
			}

			FuseId inode = i->second;
			mtp::debug("   unlinking inode ", inode.Inode);
			mtp::ObjectId id = FromFuse(inode);
			_directoryCache.erase(parent);
			_openedFiles.erase(inode);
			_objectAttrs.erase(id);
			children.erase(i);

			_session->DeleteObject(id);
			FUSE_CALL(fuse_reply_err(req, 0));
		}

		void StatFS(fuse_req_t req, FuseId ino)
		{
			mtp::scoped_mutex_lock l(_mutex);
			struct statvfs stat = { };
			stat.f_namemax = 254;

			mtp::u64 freeSpace = 0, capacity = 0;
			if (ino == FuseId::Root)
			{
				for(auto storageId : _storageIdList)
				{
					mtp::msg::StorageInfo si = _session->GetStorageInfo(storageId);
					freeSpace += si.FreeSpaceInBytes;
					capacity += si.MaxCapacity;
				}
			}
			else
			{
				mtp::StorageId storageId;
				if (IsStorage(ino))
					storageId = FuseIdToStorageId(ino);
				else
					storageId = _session->GetObjectStorage(FromFuse(ino));

				mtp::msg::StorageInfo si = _session->GetStorageInfo(storageId);
				freeSpace = si.FreeSpaceInBytes;
				capacity = si.MaxCapacity;
			}

			stat.f_frsize = stat.f_bsize = 1024 * 1024;
			stat.f_blocks = capacity / stat.f_frsize;
			stat.f_bfree = stat.f_bavail = freeSpace / stat.f_frsize;

			FUSE_CALL(fuse_reply_statfs(req, &stat));
		}
	};

	std::unique_ptr<FuseWrapper>	g_wrapper;

#define WRAP_EX(...) do { \
		try { return __VA_ARGS__ ; } \
		catch (const mtp::usb::DeviceNotFoundException &) \
		{ \
			g_wrapper->Connect(); \
			__VA_ARGS__ ; \
		} \
		catch (const std::exception &ex) \
		{ mtp::error(#__VA_ARGS__ " failed: ", ex.what()); fuse_reply_err(req, EIO); } \
	} while(false)

	void Init (void *userdata, struct fuse_conn_info *conn)
	{
		mtp::debug("Init: fuse proto version: ", conn->proto_major, ".", conn->proto_minor,
			", capability: 0x", mtp::hex(conn->capable, 8),
			", async read: ", conn->async_read,
			//", congestion_threshold: ", conn->congestion_threshold,
			//", max bg: ", conn->max_background,
			", max readahead: ", conn->max_readahead, ", max write: ", conn->max_write
		);
		try { g_wrapper->Init(userdata, conn); } catch (const std::exception &ex) { mtp::error("init failed:", ex.what()); }
	}

	void Lookup (fuse_req_t req, fuse_ino_t parent, const char *name)
	{ mtp::debug("   Lookup ", parent, " ", name); WRAP_EX(g_wrapper->Lookup(req, FuseId(parent), name)); }

	void ReadDir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
	{ mtp::debug("   Readdir ", ino, " ", size, " ", off); WRAP_EX(g_wrapper->ReadDir(req, FuseId(ino), size, off, fi)); }

	void GetAttr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
	{ mtp::debug("   GetAttr ", ino); WRAP_EX(g_wrapper->GetAttr(req, FuseId(ino), fi)); }

	void SetAttr(fuse_req_t req, fuse_ino_t ino, struct stat *attr, int to_set, struct fuse_file_info *fi)
	{ mtp::debug("   SetAttr ", ino, " 0x", mtp::hex(to_set, 8)); WRAP_EX(g_wrapper->SetAttr(req, FuseId(ino), attr, to_set, fi)); }

	void Read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
	{ mtp::debug("   Read ", ino, " ", size, " ", off); WRAP_EX(g_wrapper->Read(req, FuseId(ino), size, off, fi)); }

	void Write(fuse_req_t req, fuse_ino_t ino, const char *buf, size_t size, off_t off, struct fuse_file_info *fi)
	{ mtp::debug("   Write ", ino, " ", size, " ", off); WRAP_EX(g_wrapper->Write(req, FuseId(ino), buf, size, off, fi)); }

	void MakeNode(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode, dev_t rdev)
	{ mtp::debug("   MakeNode ", parent, " ", name, " 0x", mtp::hex(mode, 8)); WRAP_EX(g_wrapper->MakeNode(req, FuseId(parent), name, mode, rdev)); }

	void Open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
	{ mtp::debug("   Open ", ino); WRAP_EX(g_wrapper->Open(req, FuseId(ino), fi)); }

	void Release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
	{ mtp::debug("   Release ", ino); WRAP_EX(g_wrapper->Release(req, FuseId(ino), fi)); }

	void MakeDir(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode)
	{ mtp::debug("   MakeDir ", parent, " ", name, " 0x", mtp::hex(mode, 8)); WRAP_EX(g_wrapper->MakeDir(req, FuseId(parent), name, mode)); }

	void RemoveDir (fuse_req_t req, fuse_ino_t parent, const char *name)
	{ mtp::debug("   RemoveDir ", parent, " ", name); WRAP_EX(g_wrapper->RemoveDir(req, FuseId(parent), name)); }

	void Unlink(fuse_req_t req, fuse_ino_t parent, const char *name)
	{ mtp::debug("   Unlink ", parent, " ", name); WRAP_EX(g_wrapper->Unlink(req, FuseId(parent), name)); }

	void StatFS(fuse_req_t req, fuse_ino_t ino)
	{ mtp::debug("   StatFS ", ino); WRAP_EX(g_wrapper->StatFS(req, FuseId(ino))); }
}

int main(int argc, char **argv)
{
	for(int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "-odebug") == 0)
			mtp::g_debug = true;
		if (i + 1 < argc && strcmp(argv[i], "-o") == 0 && strcmp(argv[i + 1], "debug") == 0)
			mtp::g_debug = true;
	}

	try
	{ g_wrapper.reset(new FuseWrapper()); }
	catch(const std::exception &ex)
	{ mtp::error("connect failed: ", ex.what()); return 1; }

	struct fuse_lowlevel_ops ops = {};

	ops.init		= &Init;
	ops.lookup		= &Lookup;
	ops.readdir		= &ReadDir;
	ops.getattr		= &GetAttr;
	ops.setattr		= &SetAttr;
	ops.mknod		= &MakeNode;
	ops.open		= &Open;
	ops.read		= &Read;
	ops.write		= &Write;
	ops.mkdir		= &MakeDir;
	ops.release		= &Release;
	ops.rmdir		= &RemoveDir;
	ops.unlink		= &Unlink;
	ops.statfs		= &StatFS;

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct fuse_chan *ch;
	char *mountpoint;
	int err = -1;

	if (fuse_parse_cmdline(&args, &mountpoint, NULL, NULL) != -1 &&
	    (ch = fuse_mount(mountpoint, &args)) != NULL) {
		struct fuse_session *se;

		se = fuse_lowlevel_new(&args, &ops,
				       sizeof(ops), NULL);
		if (se != NULL) {
			if (fuse_set_signal_handlers(se) != -1) {
				fuse_session_add_chan(se, ch);
				err = fuse_session_loop(se);
				fuse_remove_signal_handlers(se);
				fuse_session_remove_chan(ch);
			}
			fuse_session_destroy(se);
		}
		fuse_unmount(mountpoint, ch);
	}
	fuse_opt_free_args(&args);

	return err ? 1 : 0;
}

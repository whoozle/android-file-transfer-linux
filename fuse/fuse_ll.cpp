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

		fuse_ino_t Index;

		explicit FuseId(fuse_ino_t index): Index(index) { }

		bool operator == (const FuseId &o) const
		{ return Index == o.Index; }
		bool operator != (const FuseId &o) const
		{ return !((*this) == o); }
		bool operator < (const FuseId &o) const
		{ return Index < o.Index; }
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
			ino = id.Index;
			attr.st_ino = id.Index;
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

		void SetSize(mtp::u64 size)
		{
			attr.st_size = size;
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
		time_t			_connectTime;

		typedef std::map<std::string, mtp::u32> ChildrenObjects;
		typedef std::map<mtp::u32, ChildrenObjects> Files;
		Files			_files;

		typedef std::map<mtp::u32, struct stat> ObjectAttrs;
		ObjectAttrs		_objectAttrs;

		typedef mtp::Session::ObjectEditSessionPtr ObjectEditSessionPtr;
		typedef std::map<FuseId, ObjectEditSessionPtr> OpenedFiles;
		OpenedFiles		_openedFiles;

		typedef std::map<FuseId, CharArray> DirectoryCache;
		DirectoryCache	_directoryCache;

		std::map<mtp::u32, std::string>		_storages;
		std::map<std::string, mtp::u32>		_storagesByName;

	private:
		FuseId ToFuse(mtp::u32 id)
		{ return FuseId(id + FUSE_ROOT_ID); }

		mtp::u32 FromFuse(FuseId id)
		{ return id.Index - FUSE_ROOT_ID; }

		void GetObjectInfo(ChildrenObjects &cache, mtp::u32 id)
		{
			mtp::msg::ObjectInfo oi = _session->GetObjectInfo(id);

			cache[oi.Filename] = id;

			struct stat &attr = _objectAttrs[id];
			attr.st_ino = ToFuse(id).Index;
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
				attr.st_ino = inode.Index;
				attr.st_mtime = attr.st_ctime = attr.st_atime = _connectTime;
				attr.st_mode = FuseEntry::DirectoryMode;
				return attr;
			}

			mtp::u32 id = FromFuse(inode);

			auto sit = _storages.find(id);
			if (sit != _storages.end())
			{
				struct stat attr = { };
				attr.st_ino = inode.Index;
				attr.st_mtime = attr.st_ctime = attr.st_atime = _connectTime;
				attr.st_mode = FuseEntry::DirectoryMode;
				return attr;
			}

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
				PopulateStorages();

			mtp::u32 parent = FromFuse(inode);
			{
				auto i = _files.find(parent);
				if (i != _files.end())
					return i->second;
			}

			ChildrenObjects & cache = _files[parent];

			using namespace mtp;
			if (inode == FuseId::Root)
			{
				for(const auto & i : _storagesByName) {
					cache[i.first] = i.second;
				}
				return cache;
			}

			msg::ObjectHandles oh;

			auto sit = _storages.find(parent);
			if (sit != _storages.end())
				oh = _session->GetObjectHandles(sit->first, mtp::ObjectFormat::Any, mtp::Session::Root);
			else
			{
				if (_session->GetObjectPropertyListSupported())
				{
					//populate filenames
					ByteArray data;
					{
						data = _session->GetObjectPropertyList(parent, ObjectFormat::Any, ObjectProperty::ObjectFilename, 0, 1);
						ObjectPropertyListParser<std::string> parser;
						parser.Parse(data, [&cache](u32 objectId, const std::string &name)
						{
							cache[name] = objectId;
						});
					}

					//format
					{
						data = _session->GetObjectPropertyList(parent, ObjectFormat::Any, ObjectProperty::ObjectFormat, 0, 1);
						ObjectPropertyListParser<mtp::ObjectFormat> parser;
						parser.Parse(data, [this](u32 objectId, mtp::ObjectFormat format)
						{
							struct stat & attr = _objectAttrs[objectId];
							attr.st_ino = ToFuse(objectId).Index;
							attr.st_mode = FuseEntry::GetMode(format);
						});
					}

					//size
					{
						data = _session->GetObjectPropertyList(parent, ObjectFormat::Any, ObjectProperty::ObjectSize, 0, 1);
						ObjectPropertyListParser<u64> parser;
						parser.Parse(data, [this](u32 objectId, u64 size)
						{
							_objectAttrs[objectId].st_size = size;
						});
					}

					//mtime
					try
					{
						data = _session->GetObjectPropertyList(parent, ObjectFormat::Any, ObjectProperty::DateModified, 0, 1);
						ObjectPropertyListParser<std::string> parser;
						parser.Parse(data, [this](u32 objectId, const std::string & mtime)
						{
							_objectAttrs[objectId].st_mtime = mtp::ConvertDateTime(mtime);
						});
					}
					catch(const std::exception &ex)
					{ }

					//ctime
					try
					{
						data = _session->GetObjectPropertyList(parent, ObjectFormat::Any, ObjectProperty::DateAdded, 0, 1);
						ObjectPropertyListParser<std::string> parser;
						parser.Parse(data, [this](u32 objectId, const std::string & mtime)
						{
							_objectAttrs[objectId].st_ctime = mtp::ConvertDateTime(mtime);
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
			mtp::u32 parentId = FromFuse(parentInode);
			mtp::u32 cacheParent = parentId;
			mtp::u32 storageId;
			if (_storages.find(parentId) != _storages.end())
			{
				storageId = parentId;
				parentId = mtp::Session::Root;
			}
			else
				storageId = _session->GetObjectIntegerProperty(parentId, mtp::ObjectProperty::StorageId);

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

			{ //update cache:
				auto i = _files.find(cacheParent);
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

			mtp::u32 id = FromFuse(inode);
			if (_storages.find(id) != _storages.end())
				return FuseId::Root;
			else
			{
				mtp::u64 parent = _session->GetObjectIntegerProperty(id, mtp::ObjectProperty::ParentObject);
				if (parent == 0 || parent == mtp::Session::Root) //parent == root -> storage
					parent = _session->GetObjectIntegerProperty(id, mtp::ObjectProperty::StorageId);
				return ToFuse(parent);
			}
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
			_storages.clear();
			_storagesByName.clear();
			_session.reset();
			_device.reset();
			_device = mtp::Device::Find();
			if (!_device)
				throw std::runtime_error("no MTP device found");

			_session = _device->OpenSession(1);
			_editObjectSupported = _session->EditObjectSupported();
			if (!_editObjectSupported)
				fprintf(stderr, "your device does not have android EditObject extension, mounting read-only\n");

			_connectTime = time(NULL);
			PopulateStorages();
		}

		void PopulateStorages()
		{
			_storages.clear();
			_storagesByName.clear();
			mtp::msg::StorageIDs ids = _session->GetStorageIDs();
			for(size_t i = 0; i < ids.StorageIDs.size(); ++i)
			{
				mtp::u32 id = ids.StorageIDs[i];
				mtp::msg::StorageInfo si = _session->GetStorageInfo(id);
				std::string path = (!si.StorageDescription.empty()? si.StorageDescription:  si.VolumeLabel);
				if (path.empty())
				{
					char buf[64];
					snprintf(buf, sizeof(buf), "sdcard%u", (unsigned)i);
					path = buf;
				}
				_storages[id] = path;
				_storagesByName[path] = id;
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
				if (FillEntry(entry, ToFuse(it->second)))
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
					dir.Add(data, entry.first, GetObjectAttr(ToFuse(entry.second)));
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
					_openedFiles[ino] = it->second;
				else
				{
					tr = mtp::Session::EditObject(_session, FromFuse(ino));
					_openedFiles[ino] = tr;
				}
			}
			tr->Send(off, mtp::ByteArray(buf, buf + size));
			FUSE_CALL(fuse_reply_write(req, size));
		}

		void MakeNode(fuse_req_t req, FuseId parent, const char *name, mode_t mode, dev_t rdev)
		{ mtp::scoped_mutex_lock l(_mutex); CreateObject(mtp::ObjectFormat::Undefined, req, parent, name, mode); }

		void Create(fuse_req_t req, FuseId parent, const char *name, mode_t mode, struct fuse_file_info *fi)
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

			mtp::u32 id = i->second;
			_openedFiles.erase(ToFuse(id));
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
				for(auto storage = _storages.begin(); storage != _storages.end(); ++storage)
				{
					mtp::msg::StorageInfo si = _session->GetStorageInfo(storage->first);
					freeSpace += si.FreeSpaceInBytes;
					capacity += si.MaxCapacity;
				}
			}
			else
			{
				mtp::u32 id = FromFuse(ino);
				auto sit = _storages.find(id);
				if (sit == _storages.end()) //not a storage
					id = _session->GetObjectIntegerProperty(id, mtp::ObjectProperty::StorageId);

				mtp::msg::StorageInfo si = _session->GetStorageInfo(id);
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
		{ fprintf(stderr, #__VA_ARGS__ " failed: %s\n", ex.what()); fuse_reply_err(req, EIO); } \
	} while(false)

	void Init (void *userdata, struct fuse_conn_info *conn)
	{ try { g_wrapper->Init(userdata, conn); } catch (const std::exception &ex) { mtp::error("init failed:", ex.what()); } }

	void Lookup (fuse_req_t req, fuse_ino_t parent, const char *name)
	{ WRAP_EX(g_wrapper->Lookup(req, FuseId(parent), name)); }

	void ReadDir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->ReadDir(req, FuseId(ino), size, off, fi)); }

	void GetAttr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->GetAttr(req, FuseId(ino), fi)); }

	void SetAttr(fuse_req_t req, fuse_ino_t ino, struct stat *attr, int to_set, struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->SetAttr(req, FuseId(ino), attr, to_set, fi)); }

	void Read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->Read(req, FuseId(ino), size, off, fi)); }

	void Write(fuse_req_t req, fuse_ino_t ino, const char *buf, size_t size, off_t off, struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->Write(req, FuseId(ino), buf, size, off, fi)); }

	void MakeNode(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode, dev_t rdev)
	{ WRAP_EX(g_wrapper->MakeNode(req, FuseId(parent), name, mode, rdev)); }

	void Open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->Open(req, FuseId(ino), fi)); }

	void Release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->Release(req, FuseId(ino), fi)); }

	void Create(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode, struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->Create(req, FuseId(parent), name, mode, fi)); }

	void MakeDir(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode)
	{ WRAP_EX(g_wrapper->MakeDir(req, FuseId(parent), name, mode)); }

	void RemoveDir (fuse_req_t req, fuse_ino_t parent, const char *name)
	{ WRAP_EX(g_wrapper->RemoveDir(req, FuseId(parent), name)); }

	void Unlink(fuse_req_t req, fuse_ino_t parent, const char *name)
	{ WRAP_EX(g_wrapper->Unlink(req, FuseId(parent), name)); }

	void StatFS(fuse_req_t req, fuse_ino_t ino)
	{ WRAP_EX(g_wrapper->StatFS(req, FuseId(ino))); }
}

int main(int argc, char **argv)
{
	try
	{ g_wrapper.reset(new FuseWrapper()); }
	catch(const std::exception &ex)
	{ fprintf(stderr, "%s\n", ex.what()); return 1; }

	struct fuse_lowlevel_ops ops = {};

	ops.init		= &Init;
	ops.lookup		= &Lookup;
	ops.readdir		= &ReadDir;
	ops.getattr		= &GetAttr;
	ops.setattr		= &SetAttr;
	ops.mknod		= &MakeNode;
	ops.open		= &Open;
	ops.create		= &Create;
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

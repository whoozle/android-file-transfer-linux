/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2020  Vladimir Menshakov

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <fuse/Exception.h>
#include <fuse/FuseId.h>
#include <fuse/FuseEntry.h>
#include <fuse/FuseDirectory.h>

#include <mtp/ptp/ByteArrayObjectStream.h>
#include <mtp/ptp/Device.h>
#include <mtp/ptp/ObjectPropertyListParser.h>

#include <mtp/usb/DeviceNotFoundException.h>

#include <mtp/log.h>

#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <unistd.h>
#include <vector>
#include <list>
#include <cstdio>

#include <fuse_lowlevel.h>

#ifndef RENAME_NOREPLACE
#define RENAME_NOREPLACE (1 << 0)
#endif
#ifndef RENAME_EXCHANGE
#define RENAME_EXCHANGE (1 << 1)
#endif

namespace
{
	using namespace mtp::fuse;

	class FuseWrapper
	{
		std::mutex		_mutex;
		std::string		_deviceFilter;
		bool			_claimInterface;
		bool			_resetDevice;
		mtp::DevicePtr	_device;
		mtp::SessionPtr	_session;
		bool			_editObjectSupported;
		bool			_moveObjectSupported;
		bool			_getObjectPropertyListSupported;
		bool			_getPartialObjectSupported;
		time_t			_connectTime;
		size_t			_partialObjectCacheSize;

		static constexpr size_t DefaultPartialObjectCacheSize = 3; //just 3 object entries.

		using ChildrenObjects = std::map<std::string, FuseId>;
		using Files = std::map<FuseId, ChildrenObjects>;
		Files			_files;
		std::map<fuse_ino_t, fuse_ino_t> _parentMap;

		typedef std::map<mtp::ObjectId, struct stat> ObjectAttrs;
		ObjectAttrs		_objectAttrs;

		typedef mtp::Session::ObjectEditSessionPtr ObjectEditSessionPtr;
		typedef std::map<FuseId, ObjectEditSessionPtr> OpenedFiles;
		OpenedFiles		_openedFiles;

		struct WriteBufferEntry
		{
			mtp::u64             StartOffset = 0;
			mtp::ByteArray       Data;
		};
		std::map<FuseId, WriteBufferEntry> _writeBuffers;

		fuse_ino_t      _readAheadInode = 0;
		off_t           _readAheadOffset = 0;
		mtp::ByteArray  _readAheadData;
		static constexpr size_t ReadAheadSize = 256 * 1024;

		typedef std::map<FuseId, CharArray> DirectoryCache;
		DirectoryCache	_directoryCache;

		static const size_t					MtpStorageShift = FUSE_ROOT_ID + 1;
		static const size_t					MtpObjectShift = 999998 + MtpStorageShift;

		std::vector<mtp::StorageId>					_storageIdList;
		std::map<mtp::StorageId, std::string>		_storageToName;
		std::map<std::string, mtp::StorageId>		_storageFromName;

		struct CachedStorageSpace
		{
			mtp::u64	FreeSpace;
			mtp::u64	MaxCapacity;
			time_t		Timestamp;
		};
		std::map<mtp::u32, CachedStorageSpace>		_storageSpaceCache;
		static constexpr time_t						StorageSpaceCacheTTL = 5;

		struct ObjectCacheEntry
		{
			mtp::ObjectId 	Id;
			mtp::ByteArray 	Data;

			ObjectCacheEntry()
			{ }

			ObjectCacheEntry(mtp::ObjectId id, mtp::ByteArray && data): Id(id), Data(std::move(data))
			{ }
		};

		using ObjectCache = std::list<ObjectCacheEntry>;
		ObjectCache 								_objectCache;

	private:
		static FuseId ToFuse(mtp::ObjectId id)
		{ return FuseId(id.Id + MtpObjectShift); }

		static mtp::ObjectId FromFuse(FuseId id)
		{ return mtp::ObjectId(id.Inode - MtpObjectShift); }

		static bool IsStorage(FuseId id)
		{ return id.Inode >= MtpStorageShift && id.Inode <= MtpObjectShift; }

		static void SetSize(struct stat & st, size_t size)
		{
			st.st_size = size;
			st.st_blocks = (size + 511) / 512;
		}

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

		void GetObjectInfo(ChildrenObjects &cache, mtp::ObjectId id, FuseId parentInode)
		{
			auto oi = _session->GetObjectInfo(id);

			FuseId inode = ToFuse(id);
			cache.emplace(oi.Filename, inode);
			_parentMap[inode.Inode] = parentInode.Inode;

			struct stat &attr = _objectAttrs[id];
			attr.st_uid = getuid();
			attr.st_gid = getgid();
			attr.st_ino = inode.Inode;
			attr.st_mode = FuseEntry::GetMode(oi.ObjectFormat);
			attr.st_atime = attr.st_mtime = mtp::ConvertDateTime(oi.ModificationDate);
			attr.st_ctime = mtp::ConvertDateTime(oi.CaptureDate);
			SetSize(attr, oi.ObjectCompressedSize != mtp::MaxObjectSize? oi.ObjectCompressedSize: _session->GetObjectIntegerProperty(id, mtp::ObjectProperty::ObjectSize));
			attr.st_nlink = 1;
		}

		struct stat GetObjectAttr(FuseId inode)
		{
			if (inode == FuseId::Root)
			{
				struct stat attr = { };
				attr.st_uid = getuid();
				attr.st_gid = getgid();
				attr.st_ino = inode.Inode;
				attr.st_mtime = attr.st_ctime = attr.st_atime = _connectTime;
				attr.st_mode = FuseEntry::DirectoryMode;
				attr.st_nlink = 1;
				return attr;
			}

			if (IsStorage(inode))
			{
				struct stat attr = { };
				attr.st_uid = getuid();
				attr.st_gid = getgid();
				attr.st_ino = inode.Inode;
				attr.st_mtime = attr.st_ctime = attr.st_atime = _connectTime;
				attr.st_mode = FuseEntry::DirectoryMode;
				attr.st_nlink = 1;
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

		template<typename PropertyValueType>
		void GetObjectPropertyList(mtp::ObjectId parent, const std::set<mtp::ObjectId> &originalObjectList, const mtp::ObjectProperty property,
			const std::function<void (mtp::ObjectId, const PropertyValueType &)> &callback)
		{
			std::set<mtp::ObjectId> objectList(originalObjectList);
			mtp::ByteArray data = _session->GetObjectPropertyList(parent, mtp::ObjectFormat::Any, property, 0, 1);
			mtp::ObjectPropertyListParser<PropertyValueType> parser;

			parser.Parse(data, [&objectList, &callback, property](mtp::ObjectId objectId, mtp::ObjectProperty p, const PropertyValueType & value) {
				auto it = objectList.find(objectId);
				if (property == p && it != objectList.end())
				{
					objectList.erase(it);
					try { callback(objectId, value); } catch(const std::exception &ex) { mtp::error("callback for property list 0x", mtp::hex(property, 4), " failed: ", ex.what()); }
				}
				else
					mtp::debug("extra property 0x", hex(p, 4), " returned for object ", objectId.Id, ", while querying property list 0x", mtp::hex(property, 4));
			});

			if (!objectList.empty())
			{
				mtp::error("inconsistent GetObjectPropertyList for property 0x", mtp::hex(property, 4));
				for(auto objectId : objectList)
				{
					mtp::debug("querying property 0x", mtp::hex(property, 4), " for object ", objectId);
					try
					{
						mtp::ByteArray data = _session->GetObjectProperty(objectId, property);
						mtp::InputStream stream(data);
						PropertyValueType value = PropertyValueType();
						stream >> value;
						callback(objectId, value);
					}
					catch(const std::exception &ex) { mtp::error("fallback query/callback for property 0x", mtp::hex(property, 4), " failed: ", ex.what()); }
				}
			}
		}

		ChildrenObjects & GetChildren(FuseId inode)
		{
			if (inode == FuseId::Root)
			{
				ChildrenObjects & cache = _files[inode];
				if (cache.empty())
				{
					PopulateStorages();
					for(size_t i = 0; i < _storageIdList.size(); ++i)
					{
						mtp::StorageId storageId = _storageIdList[i];
						auto name = _storageToName.find(storageId);
						if (name != _storageToName.end())
							cache.emplace(name->second, FuseId(MtpStorageShift + i));
						else
							mtp::error("no storage name for ", storageId);
					}
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
				oh = _session->GetObjectHandles(mtp::Session::AllStorages, mtp::ObjectFormat::Any, parent);

				if (_getObjectPropertyListSupported)
				{
					std::set<mtp::ObjectId> objects;
					for(auto id : oh.ObjectHandles)
						objects.insert(id);

					//populate filenames
					GetObjectPropertyList<std::string>(parent, objects, mtp::ObjectProperty::ObjectFilename,
						[this, &cache, inode](ObjectId objectId, const std::string &name)
						{
							FuseId childInode = ToFuse(objectId);
							cache.emplace(name, childInode);
							_parentMap[childInode.Inode] = inode.Inode;
						});

					//format
					GetObjectPropertyList<mtp::ObjectFormat>(parent, objects, mtp::ObjectProperty::ObjectFormat,
						[this](ObjectId objectId, mtp::ObjectFormat format)
						{
							struct stat & attr = _objectAttrs[objectId];
							attr.st_ino = ToFuse(objectId).Inode;
							attr.st_mode = FuseEntry::GetMode(format);
							attr.st_nlink = 1;
						});

					//size
					GetObjectPropertyList<mtp::u64>(parent, objects, mtp::ObjectProperty::ObjectSize,
						[this](ObjectId objectId, mtp::u64 size)
						{ SetSize(_objectAttrs[objectId], size); });

					//mtime
					try
					{
						GetObjectPropertyList<std::string>(parent, objects, mtp::ObjectProperty::DateModified,
						[this](ObjectId objectId, const std::string & mtime)
						{ _objectAttrs[objectId].st_mtime = mtp::ConvertDateTime(mtime); });
					}
					catch(const std::exception &ex)
					{ }

					//ctime
					try
					{
						GetObjectPropertyList<std::string>(parent, objects, mtp::ObjectProperty::DateAdded,
						[this](ObjectId objectId, const std::string & ctime)
						{ _objectAttrs[objectId].st_ctime = mtp::ConvertDateTime(ctime); });
					}
					catch(const std::exception &ex)
					{ }

					return cache;
				}
			}

			for(auto id : oh.ObjectHandles)
			{
				try
				{
					GetObjectInfo(cache, id, inode);
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
			mtp::debug("   creating object in storage ", mtp::hex(storageId.Id), ", parent: ", mtp::hex(parentId.Id, 8));

			mtp::msg::NewObjectInfo noi;
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
					GetObjectInfo(i->second, noi.ObjectId, parentInode); //insert object info into cache
				_directoryCache.erase(parentInode);
				_parentMap[ToFuse(noi.ObjectId).Inode] = parentInode.Inode;
				_storageSpaceCache.clear();
			}
			return ToFuse(noi.ObjectId);
		}

		void CreateObject(mtp::ObjectFormat format, fuse_req_t req, FuseId parentId, const char *name, mode_t mode, fuse_file_info *createInfo = NULL)
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

			if (createInfo)
				entry.ReplyCreate(createInfo);
			else
				entry.Reply();
		}

		FuseId GetParentObject(FuseId inode)
		{
			if (inode == FuseId::Root)
				return inode;

			if (IsStorage(inode))
				return FuseId::Root;

			auto it = _parentMap.find(inode.Inode);
			if (it != _parentMap.end())
				return FuseId(it->second);

			mtp::ObjectId id = FromFuse(inode);
			mtp::ObjectId parent = _session->GetObjectParent(id);
			fuse_ino_t parentIno;
			if (parent == mtp::Session::Device || parent == mtp::Session::Root) //parent == root -> storage
			{
				parentIno = FuseIdFromStorageId(_session->GetObjectStorage(id)).Inode;
			}
			else
				parentIno = ToFuse(parent).Inode;

			_parentMap[inode.Inode] = parentIno;
			return FuseId(parentIno);
		}

		FuseId GetStorageInode(FuseId inode)
		{
			FuseId cur = inode;
			for (int depth = 0; depth < 128 && cur != FuseId::Root; ++depth)
			{
				if (IsStorage(cur))
					return cur;
				auto it = _parentMap.find(cur.Inode);
				if (it == _parentMap.end())
					break;
				cur = FuseId(it->second);
			}
			if (cur != FuseId::Root && !IsStorage(cur))
				return FuseIdFromStorageId(_session->GetObjectStorage(FromFuse(inode)));
			return cur;
		}

		bool FillEntry(FuseEntry &entry, FuseId id)
		{
			try { entry.attr = GetObjectAttr(id); } catch(const std::exception &ex) { return false; }
			auto wb = _writeBuffers.find(id);
			if (wb != _writeBuffers.end() && !wb->second.Data.empty())
			{
				off_t pendingEnd = static_cast<off_t>(wb->second.StartOffset + wb->second.Data.size());
				if (pendingEnd > entry.attr.st_size)
					SetSize(entry.attr, pendingEnd);
			}
			entry.SetId(id);
			return true;
		}

	public:
		FuseWrapper(const std::string & deviceFilter, bool claimInterface, bool resetDevice):
			_deviceFilter(deviceFilter), _claimInterface(claimInterface), _resetDevice(resetDevice), _partialObjectCacheSize(DefaultPartialObjectCacheSize)
		{ Connect(); }

		void Connect()
		{
			mtp::scoped_mutex_lock l(_mutex);

			_openedFiles.clear();
			_writeBuffers.clear();
			_files.clear();
			_parentMap.clear();
			_objectAttrs.clear();
			_directoryCache.clear();
			_objectCache.clear();
			_storageSpaceCache.clear();
			_readAheadInode = 0;
			_readAheadOffset = 0;
			_readAheadData.clear();
			_session.reset();
			_device.reset();
			_device = mtp::Device::FindFirst(_deviceFilter, _claimInterface, _resetDevice);
			if (!_device)
				throw std::runtime_error("no MTP device found");

			_session = _device->OpenSession(1);
			const auto &di = _session->GetDeviceInfo();
			_moveObjectSupported = _session->MoveObjectSupported();
			_editObjectSupported = _session->EditObjectSupported();
			_getObjectPropertyListSupported = _session->GetObjectPropertyListSupported();
			_getPartialObjectSupported = di.Supports(mtp::OperationCode::GetPartialObject);
			mtp::print("connected: ", di.Manufacturer, " ", di.Model,
				" (MoveObject=", _moveObjectSupported ? "yes" : "no",
				", EditObject=", _editObjectSupported ? "yes" : "no",
				", GetPartialObject=", _getPartialObjectSupported ? "yes" : "no",
				", GetPartialObject64=", di.Supports(mtp::OperationCode::GetPartialObject64) ? "yes" : "no",
				", GetObjectPropertyList=", _getObjectPropertyListSupported ? "yes" : "no", ")");
			if (!_editObjectSupported)
				mtp::error("your device does not have android EditObject extension, you will not be able to write into individual files");
			if (!_getObjectPropertyListSupported)
				mtp::error("your device does not have GetObjectPropertyList extension, expect slow enumeration of big directories");
			if (!_getPartialObjectSupported)
				mtp::error("your device does not have GetPartialObject extension (beware, this will download the latest N objects you accessed and keep them in a small in-memory cache)");

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
				std::string path = si.GetName();
				if (path.empty())
				{
					char buf[64];
					std::snprintf(buf, sizeof(buf), "sdcard%u", (unsigned)i);
					path = buf;
				}
				FuseId inode(MtpStorageShift + i);
				_storageIdList.push_back(id);
				_storageFromName[path] = id;
				_storageToName[id] = path;
				_storageSpaceCache[id.Id] = { si.FreeSpaceInBytes, si.MaxCapacity, time(NULL) };
			}
		}
		std::string GetMountpoint()
		{ return _session->GetDeviceInfo().GetFilesystemFriendlyName(); }

		void Init(void *, fuse_conn_info *conn)
		{
			mtp::scoped_mutex_lock l(_mutex);
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
				if (ino == FuseId::Root && cache.empty())
				{
					CharArray data;
					dir.Add(data, ".", GetObjectAttr(ino));
					dir.Add(data, "..", GetObjectAttr(GetParentObject(ino)));
					dir.Reply(req, data, off, size);
					return;
				}

				it = _directoryCache.insert(std::make_pair(ino, CharArray())).first;
				CharArray &data = it->second;

				dir.Add(data, ".", GetObjectAttr(ino));
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

		void FlushWriteBuffer(FuseId inode)
		{
			auto it = _writeBuffers.find(inode);
			if (it == _writeBuffers.end() || it->second.Data.empty())
				return;

			mtp::u64 startOffset = it->second.StartOffset;
			mtp::ByteArray payload = std::move(it->second.Data);
			it->second.Data.clear();

			mtp::ObjectId objectId = FromFuse(inode);
			GetObjectAttr(inode);
			struct stat &attr = _objectAttrs[objectId];
			ObjectEditSessionPtr tr = GetTransaction(inode);

			off_t endOffset = static_cast<off_t>(startOffset + payload.size());
			if (endOffset > attr.st_size)
			{
				tr->Truncate(endOffset);
				SetSize(attr, endOffset);
			}

			tr->Send(startOffset, payload);
			attr.st_mtime = time(NULL);
			_storageSpaceCache.clear();
		}

		mtp::Session::ObjectEditSessionPtr GetTransaction(FuseId inode)
		{
			mtp::Session::ObjectEditSessionPtr tr;
			{
				auto it = _openedFiles.find(inode);
				if (it != _openedFiles.end())
					tr = it->second;
				else
				{
					tr = mtp::Session::EditObject(_session, FromFuse(inode));
					_openedFiles[inode] = tr;
				}
			}
			return NOT_NULL(tr);
		}

		void Read(fuse_req_t req, FuseId ino, size_t size, off_t begin, struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			ReleaseTransaction(ino);
			struct stat attr = GetObjectAttr(ino);
			off_t rsize = std::min<off_t>(attr.st_size - begin, size);
			mtp::debug("reading ", rsize, " bytes");
			mtp::ByteArray data;
			auto objectId = FromFuse(ino);
			if (rsize > 0)
			{
				if (!_getPartialObjectSupported)
				{
					std::size_t size = 0;

					ObjectCache::iterator it;
					for(it = _objectCache.begin(); it != _objectCache.end(); ++it, ++size)
					{
						auto & entry = *it;
						if (entry.Id == objectId)
						{
							mtp::debug("in-memory cache hit");
							auto src = entry.Data.data() + begin;
							std::copy(src, src + rsize, std::back_inserter(data));
							if (it != _objectCache.begin())
								std::swap(*_objectCache.begin(), *it);
							break;
						}
					}
					if (it == _objectCache.end())
					{
						mtp::debug("in-memory cache miss");
						auto stream = std::make_shared<mtp::ByteArrayObjectOutputStream>();
						_session->GetObject(objectId, stream);
						auto src = stream->GetData().data() + begin;
						std::copy(src, src + rsize, std::back_inserter(data));
						_objectCache.emplace_front(objectId, std::move(stream->GetData()));
						++size;
					}

					while (size > _partialObjectCacheSize)
					{
						mtp::debug("purging last entry from cache...");
						_objectCache.pop_back();
						--size;
					}
				} else {
					if (ino.Inode == _readAheadInode && begin >= _readAheadOffset && (begin + rsize) <= static_cast<off_t>(_readAheadOffset + _readAheadData.size()))
					{
						mtp::debug("read-ahead cache hit for inode ", ino.Inode, " at ", begin);
						auto src = _readAheadData.data() + (begin - _readAheadOffset);
						data.assign(src, src + rsize);
					}
					else
					{
						if (static_cast<size_t>(rsize) < ReadAheadSize)
						{
							off_t fetchSize = std::min<off_t>(attr.st_size - begin, ReadAheadSize);
							_readAheadData = _session->GetPartialObject(objectId, begin, fetchSize);
							_readAheadInode = ino.Inode;
							_readAheadOffset = begin;
							size_t available = std::min<size_t>(rsize, _readAheadData.size());
							data.assign(_readAheadData.data(), _readAheadData.data() + available);
						}
						else
						{
							data = _session->GetPartialObject(objectId, begin, rsize);
						}
					}
				}
			}
			mtp::debug("read ", data.size(), " bytes of data");
			if (data.empty())
			{
				FUSE_CALL(fuse_reply_buf(req, nullptr, 0));
				return;
			}
			struct fuse_bufvec bufv = {};
			bufv.count = 1;
			bufv.buf[0].size = data.size();
			bufv.buf[0].mem = data.data();
			bufv.buf[0].fd = -1;
			FUSE_CALL(fuse_reply_data(req, &bufv, FUSE_BUF_NO_SPLICE));
		}

		void Write(fuse_req_t req, FuseId inode, const char *buf, size_t size, off_t off, struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);

			if (_readAheadInode == inode.Inode)
			{
				_readAheadData.clear();
				_readAheadInode = 0;
			}
			mtp::ObjectId objectId = FromFuse(inode);
			_objectCache.remove_if([objectId](const ObjectCacheEntry &e) { return e.Id == objectId; });

			GetTransaction(inode);
			WriteBufferEntry &entry = _writeBuffers[inode];

			static constexpr size_t MaxWriteBufferSize = 512 * 1024; // 512 KiB

			if (!entry.Data.empty() && (off != static_cast<off_t>(entry.StartOffset + entry.Data.size()) || entry.Data.size() + size > MaxWriteBufferSize))
			{
				FlushWriteBuffer(inode);
			}

			if (entry.Data.empty())
			{
				entry.StartOffset = off;
			}

			if (size >= MaxWriteBufferSize)
			{
				entry.StartOffset = off;
				entry.Data.assign(buf, buf + size);
				FlushWriteBuffer(inode);
			}
			else
			{
				entry.Data.insert(entry.Data.end(), buf, buf + size);
			}

			FUSE_CALL(fuse_reply_write(req, size));
		}

		void Create(fuse_req_t req, FuseId parent, const char *name, mode_t mode, struct fuse_file_info *fi)
		{ mtp::scoped_mutex_lock l(_mutex); CreateObject(mtp::ObjectFormat::Undefined, req, parent, name, mode, fi); }

		void MakeNode(fuse_req_t req, FuseId parent, const char *name, mode_t mode, dev_t rdev)
		{ mtp::scoped_mutex_lock l(_mutex); CreateObject(mtp::ObjectFormat::Undefined, req, parent, name, mode); }

		void MakeDir(fuse_req_t req, FuseId parent, const char *name, mode_t mode)
		{ mtp::scoped_mutex_lock l(_mutex); CreateObject(mtp::ObjectFormat::Association, req, parent, name, mode); }

		void Open(fuse_req_t req, FuseId ino, struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			try
			{
				struct stat attr = GetObjectAttr(ino);
				if (attr.st_mode & S_IFDIR)
				{
					FUSE_CALL(fuse_reply_err(req, EISDIR));
					return;
				}
			}
			catch(const std::exception &ex)
			{
				FUSE_CALL(fuse_reply_err(req, ENOENT));
				return;
			}

			FUSE_CALL(fuse_reply_open(req, fi));
		}

		void ReleaseTransaction(FuseId ino)
		{
			FlushWriteBuffer(ino);
			_writeBuffers.erase(ino);
			auto i = _openedFiles.find(ino);
			if (i != _openedFiles.end())
				_openedFiles.erase(i);
		}

		void Flush(fuse_req_t req, FuseId ino, struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			int err = 0;
			try
			{
				ReleaseTransaction(ino);
			}
			catch (const mtp::usb::DeviceNotFoundException &)
			{
				_writeBuffers.erase(ino);
				_openedFiles.erase(ino);
				throw;
			}
			catch (const std::exception &ex)
			{
				mtp::error("Flush failed for inode ", ino.Inode, ": ", ex.what());
				_writeBuffers.erase(ino);
				_openedFiles.erase(ino);
				err = EIO;
			}
			FUSE_CALL(fuse_reply_err(req, err));
		}

		void Release(fuse_req_t req, FuseId ino, struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			ReleaseTransaction(ino);
			FUSE_CALL(fuse_reply_err(req, 0));
		}

		void SetAttr(fuse_req_t req, FuseId inode, struct stat *attr, int to_set, struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			FuseEntry entry(req);
			if (FillEntry(entry, inode))
			{
				if (to_set & FUSE_SET_ATTR_SIZE)
				{
					FlushWriteBuffer(inode);
					if (_readAheadInode == inode.Inode)
					{
						_readAheadData.clear();
						_readAheadInode = 0;
					}
					mtp::ObjectId id = FromFuse(inode);
					_objectCache.remove_if([id](const ObjectCacheEntry &e) { return e.Id == id; });
					off_t newSize = attr->st_size;
					ObjectEditSessionPtr tr = GetTransaction(inode);
					tr->Truncate(newSize);
					SetSize(entry.attr, newSize);
					SetSize(_objectAttrs[id], newSize);
					_storageSpaceCache.clear();
				}
				entry.ReplyAttr();
			}
			else
				entry.ReplyError(ENOENT);
		}

		void UnlinkImpl(FuseId inode)
		{
			mtp::debug("   unlinking inode ", inode.Inode);
			mtp::ObjectId id = FromFuse(inode);
			_writeBuffers.erase(inode);
			_openedFiles.erase(inode);
			_session->DeleteObject(id);
			_objectAttrs.erase(id);
			_parentMap.erase(inode.Inode);
			_files.erase(inode);
			_directoryCache.erase(inode);
			if (_readAheadInode == inode.Inode)
			{
				_readAheadData.clear();
				_readAheadInode = 0;
			}
			_objectCache.remove_if([id](const ObjectCacheEntry &e) { return e.Id == id; });
			_storageSpaceCache.clear();
		}

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
			if (GetObjectAttr(inode).st_mode & S_IFDIR)
			{
				auto &dirChildren = GetChildren(inode);
				if (!dirChildren.empty())
				{
					FUSE_CALL(fuse_reply_err(req, ENOTEMPTY));
					return;
				}
			}

			UnlinkImpl(inode);
			_directoryCache.erase(parent);
			children.erase(i);

			FUSE_CALL(fuse_reply_err(req, 0));
		}

		void RemoveDir (fuse_req_t req, FuseId parent, const char *name)
		{ Unlink(req, parent, name); }

		void Rename(fuse_req_t req, FuseId parent, const char *name, FuseId newparent, const char *newName, unsigned int flags)
		{
			mtp::debug("   Rename flags: 0x", mtp::hex(flags, 8));
			if (flags & RENAME_EXCHANGE)
			{
				FUSE_CALL(fuse_reply_err(req, EINVAL));
				return;
			}

			mtp::scoped_mutex_lock l(_mutex);
			ChildrenObjects &srcChildren = GetChildren(parent);
			auto i = srcChildren.find(name);
			if (i == srcChildren.end())
			{
				FUSE_CALL(fuse_reply_err(req, ENOENT));
				return;
			}

			if (parent == newparent && std::string(name) == std::string(newName))
			{
				FUSE_CALL(fuse_reply_err(req, 0));
				return;
			}

			FuseId inode = i->second;
			mtp::ObjectId id = FromFuse(inode);

			mtp::StorageId destStorageId;
			mtp::ObjectId destParentId;
			if (parent != newparent)
			{
				if (newparent == FuseId::Root || IsStorage(inode))
				{
					FUSE_CALL(fuse_reply_err(req, EPERM));
					return;
				}

				if (!_moveObjectSupported)
				{
					mtp::debug("MoveObject not supported by device, returning EXDEV");
					FUSE_CALL(fuse_reply_err(req, EXDEV));
					return;
				}

				if (IsStorage(newparent))
				{
					destStorageId = FuseIdToStorageId(newparent);
					destParentId = mtp::Session::Device;
				}
				else
				{
					destParentId = FromFuse(newparent);
					FuseId storageIno = GetStorageInode(newparent);
					destStorageId = IsStorage(storageIno) ? FuseIdToStorageId(storageIno) : _session->GetObjectStorage(destParentId);
				}

				FuseId srcStorageIno = GetStorageInode(parent);
				if (IsStorage(srcStorageIno) && FuseIdToStorageId(srcStorageIno) != destStorageId)
				{
					mtp::debug("Cross-storage MoveObject requested, falling back to EXDEV");
					FUSE_CALL(fuse_reply_err(req, EXDEV));
					return;
				}
			}

			ReleaseTransaction(inode);
			if (_readAheadInode == inode.Inode)
			{
				_readAheadData.clear();
				_readAheadInode = 0;
			}
			_objectCache.remove_if([id](const ObjectCacheEntry &e) { return e.Id == id; });

			ChildrenObjects &destChildren = (parent == newparent) ? srcChildren : GetChildren(newparent);
			if (parent != newparent && std::string(name) != std::string(newName)
				&& destChildren.find(name) != destChildren.end()
				&& srcChildren.find(newName) != srcChildren.end())
			{
				mtp::debug("Cross-directory MoveObject name collision in both src and dest, falling back to EXDEV");
				FUSE_CALL(fuse_reply_err(req, EXDEV));
				return;
			}

			auto old = destChildren.find(newName);
			if (old != destChildren.end())
			{
				if (flags & RENAME_NOREPLACE)
				{
					FUSE_CALL(fuse_reply_err(req, EEXIST));
					return;
				}

				if (old->second == inode)
				{
					FUSE_CALL(fuse_reply_err(req, 0));
					return;
				}

				try
				{
					struct stat srcAttr = GetObjectAttr(inode);
					struct stat destAttr = GetObjectAttr(old->second);
					bool srcIsDir = (srcAttr.st_mode & S_IFDIR) != 0;
					bool destIsDir = (destAttr.st_mode & S_IFDIR) != 0;
					if (srcIsDir && !destIsDir)
					{
						FUSE_CALL(fuse_reply_err(req, ENOTDIR));
						return;
					}
					if (!srcIsDir && destIsDir)
					{
						FUSE_CALL(fuse_reply_err(req, EISDIR));
						return;
					}
					if (destIsDir && !GetChildren(old->second).empty())
					{
						FUSE_CALL(fuse_reply_err(req, ENOTEMPTY));
						return;
					}
				}
				catch (const std::exception &)
				{ }

				mtp::debug("   unlinking target inode ", old->second.Inode);
				UnlinkImpl(old->second);
				destChildren.erase(old);
				_directoryCache.erase(newparent);
			}

			bool renamedFirst = false;
			if (parent != newparent && std::string(name) != std::string(newName) && destChildren.find(name) != destChildren.end() && srcChildren.find(newName) == srcChildren.end())
			{
				try
				{
					_session->SetObjectProperty(id, mtp::ObjectProperty::ObjectFilename, std::string(newName));
					srcChildren.erase(name);
					srcChildren.emplace(newName, inode);
					_directoryCache.erase(parent);
					renamedFirst = true;
				}
				catch (const std::exception &)
				{ }
			}

			if (parent != newparent)
			{
				try
				{
					mtp::debug("   moving inode ", inode.Inode, " (object ", id.Id, ") to parent ", destParentId.Id, " in storage ", destStorageId.Id);
					_session->MoveObject(id, destStorageId, destParentId);
				}
				catch (const std::exception &ex)
				{
					if (renamedFirst)
					{
						try
						{
							_session->SetObjectProperty(id, mtp::ObjectProperty::ObjectFilename, std::string(name));
							srcChildren.erase(newName);
							srcChildren.emplace(name, inode);
						}
						catch (const std::exception &)
						{ }
					}
					mtp::error("MoveObject failed: ", ex.what(), ", falling back to EXDEV");
					FUSE_CALL(fuse_reply_err(req, EXDEV));
					return;
				}

				const char *currentName = renamedFirst ? newName : name;
				srcChildren.erase(currentName);
				destChildren.emplace(currentName, inode);
				_parentMap[inode.Inode] = newparent.Inode;
				_directoryCache.erase(parent);
				_directoryCache.erase(newparent);
			}

			if (!renamedFirst && std::string(name) != std::string(newName))
			{
				mtp::debug("   renaming inode ", inode.Inode, " to ", newName);
				try
				{
					_session->SetObjectProperty(id, mtp::ObjectProperty::ObjectFilename, std::string(newName));
				}
				catch (const std::exception &ex)
				{
					mtp::error("SetObjectProperty(ObjectFilename) failed: ", ex.what());
					FUSE_CALL(fuse_reply_err(req, EIO));
					return;
				}
				destChildren.erase(name);
				destChildren.emplace(newName, inode);
				_directoryCache.erase(newparent);
			}

			FUSE_CALL(fuse_reply_err(req, 0));
		}

		void GetCachedStorageSpace(mtp::StorageId storageId, mtp::u64 &freeSpace, mtp::u64 &capacity)
		{
			time_t now = time(NULL);
			auto it = _storageSpaceCache.find(storageId.Id);
			if (it != _storageSpaceCache.end() && (now - it->second.Timestamp) < StorageSpaceCacheTTL)
			{
				freeSpace = it->second.FreeSpace;
				capacity = it->second.MaxCapacity;
				return;
			}
			mtp::msg::StorageInfo si = _session->GetStorageInfo(storageId);
			freeSpace = si.FreeSpaceInBytes;
			capacity = si.MaxCapacity;
			_storageSpaceCache[storageId.Id] = { freeSpace, capacity, now };
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
					mtp::u64 sFree = 0, sCap = 0;
					GetCachedStorageSpace(storageId, sFree, sCap);
					freeSpace += sFree;
					capacity += sCap;
				}
			}
			else
			{
				FuseId storageIno = IsStorage(ino) ? ino : GetStorageInode(ino);
				mtp::StorageId storageId = IsStorage(storageIno)
					? FuseIdToStorageId(storageIno)
					: _session->GetObjectStorage(FromFuse(ino));

				GetCachedStorageSpace(storageId, freeSpace, capacity);
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
			try { \
				g_wrapper->Connect(); \
				return __VA_ARGS__ ; \
			} catch (const std::exception &ex) { \
				mtp::error("reconnect failed: ", ex.what()); \
				fuse_reply_err(req, ENODEV); \
				return; \
			} \
		} \
		catch (const std::exception &ex) \
		{ mtp::error(#__VA_ARGS__ " failed: ", ex.what()); fuse_reply_err(req, EIO); } \
	} while(false)

	void Init (void *userdata, struct fuse_conn_info *conn)
	{
		mtp::debug("Init: fuse proto version: ", conn->proto_major, ".", conn->proto_minor,
			", capability: 0x", mtp::hex(conn->capable, 8),
			", max readahead: ", conn->max_readahead, ", max write: ", conn->max_write
		);

		//If synchronous reads are chosen, Fuse will wait for reads to complete before issuing any other requests.
		//mtp is completely synchronous. you cannot have two transaction in parallel, so you have to wait any operation to finish before starting another one

		conn->want &= ~FUSE_CAP_ASYNC_READ;
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

	void Create(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode, struct fuse_file_info *fi)
	{ mtp::debug("   Create ", parent, " ", name, " 0x", mtp::hex(mode, 8)); WRAP_EX(g_wrapper->Create(req, FuseId(parent), name, mode, fi)); }

	void Open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
	{ mtp::debug("   Open ", ino); WRAP_EX(g_wrapper->Open(req, FuseId(ino), fi)); }

	void Rename(fuse_req_t req, fuse_ino_t parent, const char *name, fuse_ino_t newparent, const char *newname, unsigned int flags)
	{ mtp::debug("   Rename ", parent, " ", name, " -> ", newparent, " ", newname); WRAP_EX(g_wrapper->Rename(req, FuseId(parent), name, FuseId(newparent), newname, flags)); }

	void Release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
	{ mtp::debug("   Release ", ino); WRAP_EX(g_wrapper->Release(req, FuseId(ino), fi)); }

	void Flush(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
	{ mtp::debug("   Flush ", ino); WRAP_EX(g_wrapper->Flush(req, FuseId(ino), fi)); }

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
	std::string deviceFilter;
	bool claimInterface = true;
	bool resetDevice = false;

	std::vector<char *> args;
	args.push_back(argv[0]);

	for(int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-R") == 0)
			resetDevice = true;
		else if (strcmp(argv[i], "-C") == 0)
			claimInterface = false;
		else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "-odebug") == 0)
		{
			args.push_back(argv[i]);
			mtp::g_debug = true;
		}
		else if (strcmp(argv[i], "-o") == 0 && strcmp(argv[i + 1], "debug") == 0)
		{
			if (i + 1 == argc)
			{
				mtp::error("-o requires an argument");
				return 1;
			}
			mtp::g_debug = true;
			args.push_back(argv[i]);
		}
		else if (strcmp(argv[i], "-D") == 0)
		{
			if (i + 1 == argc)
			{
				mtp::error("-D requires an argument");
				return 1;
			}
			deviceFilter = argv[i + 1];
			++i;
		}
		else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
		{
			mtp::print();
			mtp::print("Additional AFT options: ");
			mtp::print("    -R                     reset device");
			mtp::print("    -C                     do not claim USB interface");
			mtp::print("    -d / -o debug          show MTP debug output");
			mtp::print("    -D <name>              filter by manufacturer/model/serial");
			mtp::print("    -v                     AFT verbose output");
			return 0;
		} else
			args.push_back(argv[i]);
	}

	args.push_back(nullptr);

	try
	{ g_wrapper.reset(new FuseWrapper(deviceFilter, claimInterface, resetDevice)); }
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
	ops.create		= &Create;
	ops.read		= &Read;
	ops.write		= &Write;
	ops.mkdir		= &MakeDir;
	ops.rename		= &Rename;
	ops.release		= &Release;
	ops.flush		= &Flush;
	ops.rmdir		= &RemoveDir;
	ops.unlink		= &Unlink;
	ops.statfs		= &StatFS;

	struct fuse_args fuse_args = FUSE_ARGS_INIT(static_cast<int>(args.size() - 1), args.data());
	struct fuse_cmdline_opts opts = {};
	int err = -1;

	if (fuse_parse_cmdline(&fuse_args, &opts) == 0)
	{
		if (opts.show_version)
		{
			fuse_lowlevel_version();
			exit(0);
		}
		if (opts.show_help)
		{
			fuse_cmdline_help();
			fuse_lowlevel_help();
			exit(0);
		}

	    if (opts.mountpoint != NULL)
		{
			struct fuse_session *se = fuse_session_new(&fuse_args, &ops, sizeof(ops), NULL);
			if (se != NULL)
			{
				if (fuse_set_signal_handlers(se) == 0)
				{
					if (fuse_session_mount(se, opts.mountpoint) == 0)
					{
						if (fuse_daemonize(opts.foreground) == -1)
							perror("fuse_daemonize");
						if (opts.singlethread)
							err = fuse_session_loop(se);
						else
							err = fuse_session_loop_mt(se, nullptr);

						fuse_session_unmount(se);
					}
					fuse_remove_signal_handlers(se);
				}
				fuse_session_destroy(se);
			}
		}
	} else {
		mtp::error("fuse_parse_cmdline failed");
	}
	free(opts.mountpoint);
	fuse_opt_free_args(&fuse_args);

	return err ? 1 : 0;
}

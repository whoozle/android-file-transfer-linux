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

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <mtp/ptp/Device.h>
#include <mtp/ptp/ByteArrayObjectStream.h>
#include <mtp/usb/DeviceNotFoundException.h>
#include <mtp/ptp/ObjectPropertyListParser.h>

#include <map>
#include <string>

#define USE_OBJECT_INFO 1

namespace
{
	class FuseWrapper
	{
		struct FileInfo
		{
			std::vector<mtp::u32> Files;
		};

		std::mutex		_mutex;
		mtp::DevicePtr	_device;
		mtp::SessionPtr	_session;
		bool			_editObjectSupported;

		typedef std::map<std::string, mtp::u32> Files;
		Files			_files;

		typedef mtp::Session::ObjectEditSessionPtr ObjectEditSessionPtr;
		typedef std::map<mtp::u32, ObjectEditSessionPtr> OpenedFiles;
		OpenedFiles		_openedFiles;

		typedef std::map<std::string, mtp::u32> Storages;
		Storages		_storages;

	public:
		FuseWrapper()
		{ Connect(); }

		void Connect()
		{
			mtp::scoped_mutex_lock l(_mutex);

			_openedFiles.clear();
			_files.clear();
			_storages.clear();
			_session.reset();
			_device.reset();
			_device = mtp::Device::Find();
			if (!_device)
				throw std::runtime_error("no MTP device found");

			_session = _device->OpenSession(1);
			_editObjectSupported = _session->EditObjectSupported();
			if (!_editObjectSupported)
				fprintf(stderr, "your device does not have android EditObject extension, mounting read-only\n");

			mtp::msg::StorageIDs ids = _session->GetStorageIDs();
			for(size_t i = 0; i < ids.StorageIDs.size(); ++i)
			{
				mtp::u32 id = ids.StorageIDs[i];
				mtp::msg::StorageInfo si = _session->GetStorageInfo(id);
				std::string path = "/" + (!si.StorageDescription.empty()? si.StorageDescription:  si.VolumeLabel);
				if (path.empty())
				{
					char buf[64];
					snprintf(buf, sizeof(buf), "sdcard%u", (unsigned)i);
					path = buf;
				}
				_storages[path] = id;
			}
		}

		void * Init (struct fuse_conn_info *conn)
		{
			conn->want |= conn->capable & FUSE_CAP_BIG_WRITES; //big writes
			static const size_t MaxWriteSize = 1024 * 1024;
			if (conn->max_write < MaxWriteSize)
				conn->max_write = MaxWriteSize;
			return NULL;
		}

		int GetAttr(const char *path_, struct stat *stbuf)
		{
			std::string path(path_);
			if (path == "/")
			{
				stbuf->st_ino = 1;
				stbuf->st_mode = S_IFDIR | 0755;
				stbuf->st_nlink = 2;
				return 0;
			}

			mtp::scoped_mutex_lock l(_mutex);
			auto i = _storages.find(path);
			if (i != _storages.end())
			{
				stbuf->st_ino = i->second;
				stbuf->st_mode = S_IFDIR | 0755;
				stbuf->st_nlink = 2;
				return 0;
			}

			mtp::u32 id = Resolve(path);
			if (id)
			{
				stbuf->st_mode = 0644;
				stbuf->st_ino = id;
				stbuf->st_ctim = timespec();
				stbuf->st_mtim = timespec();
				stbuf->st_nlink = 1;
#if USE_OBJECT_INFO
				mtp::msg::ObjectInfo oi = _session->GetObjectInfo(id);
				if (oi.ObjectFormat == mtp::ObjectFormat::Association)
					stbuf->st_mode |= S_IFDIR | 0111;
				else
					stbuf->st_mode |= S_IFREG;

				stbuf->st_ctim.tv_sec = mtp::ConvertDateTime(oi.CaptureDate);
				stbuf->st_mtim.tv_sec = mtp::ConvertDateTime(oi.ModificationDate);
				stbuf->st_size = oi.ObjectCompressedSize != mtp::MaxObjectSize? oi.ObjectCompressedSize: _session->GetObjectIntegerProperty(id, mtp::ObjectProperty::ObjectSize);
#else
				mtp::ObjectFormat format = static_cast<mtp::ObjectFormat>(_session->GetObjectIntegerProperty(id, mtp::ObjectProperty::ObjectFormat));
				if (format == mtp::ObjectFormat::Association)
					stbuf->st_mode |= S_IFDIR | 0111;
				else
					stbuf->st_mode |= S_IFREG;

				//date created, date added always returns 0, date created is unsupported on android
				stbuf->st_size = _session->GetObjectIntegerProperty(id, mtp::ObjectProperty::ObjectSize);
#endif
				return 0;
			}
			return -ENOENT;
		}

		void Append(const std::string &path, mtp::u32 id, const std::string &name, void * buf, fuse_fill_dir_t filler)
		{
			struct stat fileInfo = { };
			fileInfo.st_ino = id;
			if (filler)
				filler(buf, name.c_str(), &fileInfo, 0);
			_files[path + "/" + name] = id;
		}

		void Append(const std::string &path, const mtp::msg::ObjectHandles &handles, void * buf, fuse_fill_dir_t filler)
		{
			for(auto & id : handles.ObjectHandles)
			{
				try
				{
					std::string name = _session->GetObjectStringProperty(id, mtp::ObjectProperty::ObjectFilename);
					Append(path, id, name, buf, filler);
				} catch(const std::exception &ex)
				{ }
			}
		}

		void AppendAllSubobjects(const std::string &subpath, mtp::u32 parent, void * buf, fuse_fill_dir_t filler)
		{
			using namespace mtp;
			if (_session->GetObjectPropertyListSupported())
			{
				ByteArray data = _session->GetObjectPropertyList(parent, ObjectFormat::Any, ObjectProperty::ObjectFilename, 0, 1);
				ObjectPropertyListParser<std::string> parser;
				parser.Parse(data, [this, &subpath, buf, filler](u32 objectId, const std::string &name)
				{
					Append(subpath, objectId, name, buf, filler);
				});
			}
			else
			{
				msg::ObjectHandles list = _session->GetObjectHandles(mtp::Session::AllStorages, mtp::Session::AllFormats, parent);
				Append(subpath, list, buf, filler);
			}
		}

		mtp::u32 Resolve(const std::string &path)
		{
			auto file = _files.find(path);
			if (file != _files.end())
				return file->second;

			// /STORAGE/dir1/dir2/file

			size_t idx = 0;
			while(idx < path.size())
			{
				size_t next = path.find('/', idx + 1);
				std::string subpath(path.substr(0, next));

				auto storage = _storages.find(subpath);
				if (storage != _storages.end())
				{
					mtp::msg::ObjectHandles list = _session->GetObjectHandles(storage->second, mtp::Session::AllFormats, mtp::Session::Root);
					Append(subpath, list, 0, 0);
				}
				else
				{
					auto file = _files.find(subpath);
					if (file != _files.end())
					{
						mtp::msg::ObjectHandles list = _session->GetObjectHandles(mtp::Session::AllStorages, mtp::Session::AllFormats, file->second);
						AppendAllSubobjects(subpath, file->second, 0, 0);
					}
					else
						return 0;
				}
				if (next == path.npos)
					break;
				else
					idx = next;
			}

			file = _files.find(path);
			if (file != _files.end())
				return file->second;

			return 0;
		}

		int ReadDir(const char *path_, void *buf, fuse_fill_dir_t filler,
					 off_t offset, struct fuse_file_info *fi)
		{
			std::string path(path_);
			mtp::scoped_mutex_lock l(_mutex);
			if (path == "/")
			{
				filler(buf, ".", NULL, 0);
				filler(buf, "..", NULL, 0);

				for(auto & r : _storages)
				{
					filler(buf, r.first.c_str() + 1, NULL, 0);
				}
				return 0;
			}

			auto storage = _storages.find(path);
			if (storage != _storages.end())
			{
				filler(buf, ".", NULL, 0);
				filler(buf, "..", NULL, 0);

				mtp::msg::ObjectHandles list = _session->GetObjectHandles(storage->second, mtp::Session::AllFormats, mtp::Session::Root);
				Append(path, list, buf, filler);
				return 0;
			}

			mtp::u32 id = Resolve(path);

			if (id)
			{
				filler(buf, ".", NULL, 0);
				filler(buf, "..", NULL, 0);

				AppendAllSubobjects(path, id, buf, filler);
				return 0;
			}

			return -ENOENT;
		}

		int Unlink (const char *path_)
		{
			std::string path(path_);
			mtp::scoped_mutex_lock l(_mutex);
			mtp::u32 id = Resolve(path);
			if (!id)
				return -ENOENT;
			_session->DeleteObject(id);
			_files.erase(path);
			return 0;
		}

		int ResolveParent(const std::string &path, std::string &filename, mtp::u32 &storageId, mtp::u32 &parentId)
		{
			size_t parentPos = path.rfind('/');
			if (parentPos == path.npos)
				return -ENOENT;
			if (parentPos == 0)
				return -EACCES;

			size_t storagePos = path.find('/', 1);
			if (storagePos == path.npos)
				return -EACCES;

			std::string storage = path.substr(0, storagePos);
			auto i = _storages.find(storage);
			if (i != _storages.end())
			{
				storageId = i->second;
			}
			else
				return -ENOENT;

			std::string parent = path.substr(0, parentPos);
			parentId = parent != storage? Resolve(parent): mtp::Session::Root;
			//printf("resolve parent %s -> %s %s %u %u\n", path.c_str(), storage.c_str(), parent.c_str(), storageId, parentId);

			if (storageId == 0 || parentId == 0)
				return -ENOENT;

			filename = path.substr(parentPos + 1);
			return 0;
		}

		int MakeDir (const char *path_, mode_t mode)
		{
			std::string path(path_);

			mtp::scoped_mutex_lock l(_mutex);
			mtp::u32 storageId, parentId;
			std::string filename;
			int r = ResolveParent(path, filename, storageId, parentId);
			if (r)
				return r;

			_session->CreateDirectory(filename, parentId, storageId);
			return 0;
		}

		int RemoveDir (const char *path)
		{ return Unlink(path); }

		int Create(const char *path_, mode_t mode, struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			if (!_editObjectSupported)
				return -EROFS;

			std::string path(path_);
			mtp::u32 storageId, parentId;
			std::string filename;
			int r = ResolveParent(path, filename, storageId, parentId);
			if (r)
				return r;

			mtp::msg::ObjectInfo oi;
			oi.Filename = filename;
			oi.ObjectFormat = mtp::ObjectFormatFromFilename(filename);
			_session->SendObjectInfo(oi, storageId, parentId);
			_session->SendObject(std::make_shared<mtp::ByteArrayObjectInputStream>(mtp::ByteArray()));
			return 0;
		}

		int Open(const char *path, struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			return Resolve(path)? 0: -ENOENT;
		}

		int Read(const char *path, char *buf, size_t size, off_t offset,
					  struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			mtp::u32 id = Resolve(path);
			if (!id)
				return -ENOENT;

			mtp::ByteArray data = _session->GetPartialObject(id, offset, size);
			if (data.size() > size)
				throw std::runtime_error("too much data");
			std::copy(data.begin(), data.end(), buf);
			return data.size();
		}

		ObjectEditSessionPtr GetSession(mtp::u32 id)
		{
			mtp::scoped_mutex_lock l(_mutex);
			auto openedFileSession = _openedFiles.find(id);
			if (openedFileSession != _openedFiles.end())
				return openedFileSession->second;
			else
				return _openedFiles[id] = mtp::Session::EditObject(_session, id);
		}

		int Flush(const char *path, struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			mtp::u32 id = Resolve(path);
			if (!id)
				return -ENOENT;
			_openedFiles.erase(id);
			return 0;
		}

		int Write(const char *path, const char *buf, size_t size, off_t offset,
					  struct fuse_file_info *fi)
		{
			mtp::u32 id;
			{
				mtp::scoped_mutex_lock l(_mutex);
				if (!_editObjectSupported)
					return -EROFS;
				id = Resolve(path);
				if (!id)
					return -ENOENT;
			}

			ObjectEditSessionPtr edit = GetSession(id);
			edit->Send(offset, mtp::ByteArray(buf, buf + size));
			return size;
		}

		int Truncate(const char *path, off_t offset)
		{
			mtp::u32 id;
			{
				mtp::scoped_mutex_lock l(_mutex);
				if (!_editObjectSupported)
					return -EROFS;
				id = Resolve(path);
				if (!id)
					return -ENOENT;
			}

			ObjectEditSessionPtr edit = GetSession(id);
			edit->Truncate(offset);
			return 0;
		}

		int StatFS (const char *path_, struct statvfs *stat)
		{
			stat->f_namemax = 254;

			mtp::u64 freeSpace = 0, capacity = 0;
			std::string path(path_);
			mtp::scoped_mutex_lock l(_mutex);
			if (path != "/")
			{
				std::string storage = path.substr(0, path.find('/', 1));
				auto i = _storages.find(storage);
				if (i == _storages.end())
					return -ENOENT;

				mtp::msg::StorageInfo si = _session->GetStorageInfo(i->second);
				freeSpace = si.FreeSpaceInBytes;
				capacity = si.MaxCapacity;
			}
			else
			{
				for(auto storage = _storages.begin(); storage != _storages.end(); ++storage)
				{
					mtp::msg::StorageInfo si = _session->GetStorageInfo(storage->second);
					freeSpace += si.FreeSpaceInBytes;
					capacity += si.MaxCapacity;
				}
			}
			stat->f_frsize = stat->f_bsize = 1024 * 1024;
			stat->f_blocks = capacity / stat->f_frsize;
			stat->f_bfree = stat->f_bavail = freeSpace / stat->f_frsize;
			if (!_editObjectSupported)
				stat->f_flag |= ST_RDONLY;
			return 0;
		}

		int SetTimes(const char *path, const struct timespec tv[2])
		{
			mtp::scoped_mutex_lock l(_mutex);
			mtp::u32 id = Resolve(path);
			if (!id)
				return -ENOENT;

			std::string mtime = mtp::ConvertDateTime(tv[1].tv_sec);
#if 0
			std::string atime = mtp::ConvertDateTime(tv[0].tv_sec);

			try { _session->SetObjectProperty(id, mtp::ObjectProperty::DateCreated, mtime); }
			catch(const mtp::InvalidResponseException &ex)
			{
				if (ex.Type != mtp::ResponseType::ObjectPropNotSupported)
					throw;
			}
#endif
			try { _session->SetObjectProperty(id, mtp::ObjectProperty::DateModified, mtime); }
			catch(const mtp::InvalidResponseException &ex)
			{
				if (ex.Type != mtp::ResponseType::ObjectPropNotSupported)
					throw;
			}
			return 0;
		}

		int ChangeMode(const char *path, mode_t mode)
		{
			mtp::scoped_mutex_lock l(_mutex);
			mtp::u32 id = Resolve(path);
			if (!id)
				return -ENOENT;

			return 0;
		}
	};

	std::unique_ptr<FuseWrapper>	g_wrapper;

#define WRAP_EX(...) do { \
		try { return __VA_ARGS__ ; } \
		catch (const mtp::usb::DeviceNotFoundException &) \
		{ \
			g_wrapper->Connect(); \
			return __VA_ARGS__ ; \
		} \
		catch (const std::exception &ex) \
		{ fprintf(stderr, #__VA_ARGS__ " failed: %s\n", ex.what()); return -EIO; } \
	} while(false)

	void * Init (struct fuse_conn_info *conn)
	{ try { return g_wrapper->Init(conn); } catch (const std::exception &ex) { return NULL; } }

	int GetAttr(const char *path, struct stat *stbuf)
	{ WRAP_EX(g_wrapper->GetAttr(path, stbuf)); }

	int ReadDir(const char *path, void *buf, fuse_fill_dir_t filler,
				 off_t offset, struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->ReadDir(path, buf, filler, offset, fi)); }

	int Open(const char *path, struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->Open(path, fi)); }

	int Create(const char *path, mode_t mode, struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->Create(path, mode, fi)); }

	int Read(const char *path, char *buf, size_t size, off_t offset,
				  struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->Read(path, buf, size, offset, fi)); }

	int Write(const char *path, const char *buf, size_t size, off_t offset,
				  struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->Write(path, buf, size, offset, fi)); }

	int Flush (const char *path, struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->Flush(path, fi)); }

	int Unlink (const char *path)
	{ WRAP_EX(g_wrapper->Unlink(path)); }

	int MakeDir (const char *path, mode_t mode)
	{ WRAP_EX(g_wrapper->MakeDir(path, mode)); }

	int RemoveDir (const char *path)
	{ WRAP_EX(g_wrapper->RemoveDir(path)); }

	int Truncate(const char *path, off_t offset)
	{ WRAP_EX(g_wrapper->Truncate(path, offset)); }

	int StatFS (const char *path, struct statvfs *stat)
	{ WRAP_EX(g_wrapper->StatFS(path, stat)); }

	//int SetTimes(const char *path, const struct timespec tv[2])
	//{ WRAP_EX(g_wrapper->SetTimes(path, tv)); }

	int ChangeMode (const char *path, mode_t mode)
	{ WRAP_EX(g_wrapper->ChangeMode(path, mode)); }
}

int main(int argc, char **argv)
{
	try
	{ g_wrapper.reset(new FuseWrapper()); }
	catch(const std::exception &ex)
	{ fprintf(stderr, "%s\n", ex.what()); return 1; }

	struct fuse_operations ops = {};

	ops.init		= &Init;
	ops.getattr		= &GetAttr;
	ops.readdir		= &ReadDir;
	ops.open		= &Open;
	ops.create		= &Create;
	ops.read		= &Read;
	ops.write		= &Write;
	ops.flush		= &Flush;
	ops.mkdir		= &MakeDir;
	ops.rmdir		= &RemoveDir;
	ops.unlink		= &Unlink;
	ops.truncate	= &Truncate;
	ops.statfs		= &StatFS;
	//ops.utimens		= &SetTimes; //this is slow, access time is not supported by MTP, DateModified property does not work reliably.
	ops.chmod		= &ChangeMode;

	return fuse_main(argc, argv, &ops, NULL);
}

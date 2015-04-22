#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <mtp/ptp/Device.h>
#include <mtp/ptp/ByteArrayObjectStream.h>

#include <map>
#include <string>

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

		typedef std::map<std::string, mtp::u32> Files;
		Files			_files;

		typedef mtp::Session::ObjectEditSessionPtr ObjectEditSessionPtr;
		typedef std::map<mtp::u32, ObjectEditSessionPtr> OpenedFiles;
		OpenedFiles		_openedFiles;

		typedef std::map<std::string, mtp::u32> Storages;
		Storages		_storages;

	public:
		FuseWrapper(mtp::DevicePtr device): _device(device), _session(device->OpenSession(1))
		{
			mtp::msg::StorageIDs ids = _session->GetStorageIDs();
			for(size_t i = 0; i < ids.StorageIDs.size(); ++i)
			{
				mtp::u32 id = ids.StorageIDs[i];
				mtp::msg::StorageInfo si = _session->GetStorageInfo(id);
				std::string path = "/" + (!si.VolumeLabel.empty()? si.VolumeLabel: si.StorageDescription);
				if (!path.empty())
					_storages[path] = id;
			}
		}

		int GetAttr(const char *path_, struct stat *stbuf)
		{
			std::string path(path_);
			if (path == "/")
			{
				stbuf->st_mode = S_IFDIR | 0755;
				stbuf->st_nlink = 2;
				return 0;
			}

			mtp::scoped_mutex_lock l(_mutex);
			Storages::const_iterator i = _storages.find(path);
			if (i != _storages.end())
			{
				stbuf->st_mode = S_IFDIR | 0755;
				stbuf->st_nlink = 2;
				return 0;
			}

			mtp::u32 id = Resolve(path);
			if (id)
			{
				stbuf->st_mode = 0644;

				mtp::msg::ObjectInfo oi = _session->GetObjectInfo(id);
				if (oi.ObjectFormat == mtp::ObjectFormat::Association)
					stbuf->st_mode |= S_IFDIR | 0111;
				else
					stbuf->st_mode |= S_IFREG;

				stbuf->st_ctim = timespec();
				stbuf->st_ctim.tv_sec = mtp::ConvertDateTime(oi.CaptureDate);
				stbuf->st_mtim = timespec();
				stbuf->st_mtim.tv_sec = mtp::ConvertDateTime(oi.ModificationDate);
				stbuf->st_nlink = 1;
				stbuf->st_size = oi.ObjectCompressedSize != mtp::MaxObjectSize? oi.ObjectCompressedSize: _session->GetObjectIntegerProperty(id, mtp::ObjectProperty::ObjectSize);
				return 0;
			}
			return -ENOENT;
		}

		void Append(const std::string &path, const mtp::msg::ObjectHandles &handles, void * buf, fuse_fill_dir_t filler)
		{
			for(auto & id : handles.ObjectHandles)
			{
				try
				{
					std::string name = _session->GetObjectStringProperty(id, mtp::ObjectProperty::ObjectFilename);
					_files[path + "/" + name] = id;
					if (filler)
						filler(buf, name.c_str(), NULL, 0);
				} catch(const std::exception &ex)
				{ }
			}
		}

		mtp::u32 Resolve(const std::string &path)
		{
			Files::const_iterator file = _files.find(path);
			if (file != _files.end())
				return file->second;

			// /STORAGE/dir1/dir2/file

			size_t idx = 0;
			while(idx < path.size())
			{
				size_t next = path.find('/', idx + 1);
				std::string subpath(path.substr(0, next));

				Storages::const_iterator storage = _storages.find(subpath);
				if (storage != _storages.end())
				{
					mtp::msg::ObjectHandles list = _session->GetObjectHandles(storage->second, mtp::Session::AllFormats, mtp::Session::Root);
					Append(subpath, list, 0, 0);
				}
				else
				{
					Files::const_iterator file = _files.find(subpath);
					if (file != _files.end())
					{
						mtp::msg::ObjectHandles list = _session->GetObjectHandles(mtp::Session::AllStorages, mtp::Session::AllFormats, file->second);
						Append(subpath, list, 0, 0);
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

			Storages::const_iterator storage = _storages.find(path);
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

				mtp::msg::ObjectHandles list = _session->GetObjectHandles(mtp::Session::AllStorages, mtp::Session::AllFormats, id);
				Append(path, list, buf, filler);
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
			Storages::const_iterator i = _storages.find(storage);
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

			mtp::msg::ObjectInfo oi;
			oi.Filename = filename;
			oi.ObjectFormat = mtp::ObjectFormat::Association;
			oi.AssociationType = 1;
			_session->SendObjectInfo(oi, storageId, parentId);
			return 0;
		}

		int RemoveDir (const char *path)
		{ return Unlink(path); }

		int Create(const char *path_, mode_t mode, struct fuse_file_info *fi)
		{
			std::string path(path_);
			mtp::scoped_mutex_lock l(_mutex);
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
			OpenedFiles::const_iterator i = _openedFiles.find(id);
			if (i != _openedFiles.end())
				return i->second;
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
				id = Resolve(path);
				if (!id)
					return -ENOENT;
			}

			ObjectEditSessionPtr edit = GetSession(id);
			edit->Truncate(offset);
			return 0;
		}

		int StatFS (const char *path, struct statvfs *stat)
		{
			stat->f_namemax = 254;
			return 0;
		}

		int SetTimes(const char *path, const struct timespec tv[2])
		{
			mtp::scoped_mutex_lock l(_mutex);
			mtp::u32 id = Resolve(path);
			if (!id)
				return -ENOENT;

			std::string ctime = mtp::ConvertDateTime(tv[1].tv_sec);
			std::string mtime = mtp::ConvertDateTime(tv[1].tv_sec);
			try { _session->SetObjectProperty(id, mtp::ObjectProperty::DateCreated, mtime); }
			catch(const mtp::InvalidResponseException &ex)
			{
				if (ex.Type != mtp::ResponseType::ObjectPropNotSupported)
					throw;
			}
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

	mtp::DevicePtr					g_device;
	std::unique_ptr<FuseWrapper>	g_wrapper;

#define WRAP_EX(...) do { \
		try { return __VA_ARGS__ ; } \
		catch (const std::exception &ex) \
		{ fprintf(stderr, #__VA_ARGS__ " failed: %s", ex.what()); return -EIO; } \
	} while(false)

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

	int SetTimes(const char *path, const struct timespec tv[2])
	{ WRAP_EX(g_wrapper->SetTimes(path, tv)); }

	int ChangeMode (const char *path, mode_t mode)
	{ WRAP_EX(g_wrapper->ChangeMode(path, mode)); }
}

int main(int argc, char **argv)
{
	try
	{
		g_device = mtp::Device::Find();
		if (!g_device)
			throw std::runtime_error("no MTP device found");
		g_wrapper.reset(new FuseWrapper(g_device));
	} catch(const std::exception &ex)
	{ printf("%s\n", ex.what()); return 1; }

	struct fuse_operations ops = {};

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
	ops.utimens		= &SetTimes;
	ops.chmod		= &ChangeMode;

	return fuse_main(argc, argv, &ops, NULL);
}

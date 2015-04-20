#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <mtp/ptp/Device.h>

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
				stbuf->st_mode = 0755;

				mtp::ObjectFormat format((mtp::ObjectFormat)_session->GetObjectIntegerProperty(id, mtp::ObjectProperty::ObjectFormat));
				if (format == mtp::ObjectFormat::Association)
					stbuf->st_mode |= S_IFDIR;
				else
					stbuf->st_mode |= S_IFREG;

				stbuf->st_nlink = 1;
				stbuf->st_size = _session->GetObjectIntegerProperty(id, mtp::ObjectProperty::ObjectSize);
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

		int Open(const char *path, struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			return -ENOENT;
		}

		int Read(const char *path, char *buf, size_t size, off_t offset,
					  struct fuse_file_info *fi)
		{
			mtp::scoped_mutex_lock l(_mutex);
			return -ENOENT;
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

	int Read(const char *path, char *buf, size_t size, off_t offset,
				  struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->Read(path, buf, size, offset, fi)); }

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
	{ fprintf(stderr, "%s\n", ex.what()); return 1; }

	struct fuse_operations ops = {};

	ops.getattr	= &GetAttr;
	ops.readdir	= &ReadDir;
	ops.open	= &Open;
	ops.read	= &Read;

	return fuse_main(argc, argv, &ops, NULL);
}

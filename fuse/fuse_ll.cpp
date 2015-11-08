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

	struct FuseEntry : fuse_entry_param
	{
		static constexpr const double Timeout = 60.0;

		fuse_req_t Request;

		FuseEntry(fuse_req_t req): fuse_entry_param(), Request(req)
		{
			generation = 1;
			attr_timeout = entry_timeout = Timeout;
		};

		void SetFileMode()
		{ attr.st_mode = S_IFREG | 0444; }

		void SetDirectoryMode()
		{ attr.st_mode = S_IFDIR | 0755; }

		void SetFormat(mtp::ObjectFormat format)
		{
			switch(format)
			{
			case mtp::ObjectFormat::Association:
				SetDirectoryMode();
				break;
			default:
				SetFileMode();
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
		std::vector<char>	Data;

		FuseDirectory(fuse_req_t request): Request(request) { Data.reserve(4096); }

		void Add(mtp::u32 id, const std::string &name)
		{
			size_t size = fuse_add_direntry(Request, NULL, 0, name.c_str(), NULL, 0);
			size_t offset = Data.size();
			Data.resize(Data.size() + size);
			struct stat entry = { };
			entry.st_ino = id;
			fuse_add_direntry(Request, Data.data() + offset, size, name.c_str(), &entry, Data.size());
		}

		void Reply(off_t off, size_t size)
		{
			if ((size_t)off < size)
				fuse_reply_buf(Request, Data.data() + off, std::min<size_t>(size, Data.size() - off));
			else
				fuse_reply_buf(Request, NULL, 0);
		}
	};

	class FuseWrapper
	{
		std::mutex		_mutex;
		mtp::DevicePtr	_device;
		mtp::SessionPtr	_session;
		bool			_editObjectSupported;

		typedef std::map<std::string, mtp::u32> ChildrenObjects;
		typedef std::map<mtp::u32, ChildrenObjects> Files;
		Files			_files;

		typedef std::map<mtp::u32, mtp::ObjectFormat> ObjectTypes;
		ObjectTypes		_objectFormats;

		typedef mtp::Session::ObjectEditSessionPtr ObjectEditSessionPtr;
		typedef std::map<mtp::u32, ObjectEditSessionPtr> OpenedFiles;
		OpenedFiles		_openedFiles;

		std::map<mtp::u32, std::string>		_storages;
		std::map<std::string, mtp::u32>		_storagesByName;

	public:
		FuseWrapper()
		{ Connect(); }

		void Connect()
		{
			mtp::scoped_mutex_lock l(_mutex);

			_openedFiles.clear();
			_files.clear();
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
			conn->want |= conn->capable & FUSE_CAP_BIG_WRITES; //big writes
			static const size_t MaxWriteSize = 1024 * 1024;
			if (conn->max_write < MaxWriteSize)
				conn->max_write = MaxWriteSize;
		}

		mtp::ObjectFormat GetObjectFormat(mtp::u32 id)
		{
			auto i = _objectFormats.find(id);
			if (i != _objectFormats.end())
				return i->second;
			mtp::ObjectFormat format = static_cast<mtp::ObjectFormat>(_session->GetObjectIntegerProperty(id, mtp::ObjectProperty::ObjectFormat));
			_objectFormats.insert(std::make_pair(id, format));
			return format;
		}

		ChildrenObjects & GetChildren(mtp::u32 parent)
		{
			{
				auto i = _files.find(parent);
				if (i != _files.end())
					return i->second;
			}

			ChildrenObjects & cache = _files[parent];
			using namespace mtp;

			msg::ObjectHandles oh;

			auto sit = _storages.find(parent);
			if (sit != _storages.end())
				oh = _session->GetObjectHandles(sit->first, mtp::ObjectFormat::Any, mtp::Session::Root);
			else
			{
				if (_session->GetObjectPropertyListSupported())
				{
					ByteArray data;
					{
						data = _session->GetObjectPropertyList(parent, ObjectFormat::Any, ObjectProperty::ObjectFilename, 0, 1);
						ObjectPropertyListParser<std::string> parser;
						parser.Parse(data, [&cache](u32 objectId, const std::string &name)
						{
							cache[name] = objectId;
						});
					}
					{
						data = _session->GetObjectPropertyList(parent, ObjectFormat::Any, ObjectProperty::ObjectFormat, 0, 1);
						ObjectPropertyListParser<u16> parser;
						parser.Parse(data, [this](u32 objectId, u16 format)
						{
							_objectFormats[objectId] = static_cast<ObjectFormat>(format);
						});
					}
					return cache;
				}
				oh = _session->GetObjectHandles(mtp::Session::AllStorages, mtp::ObjectFormat::Any, parent);
			}

			for(auto id : oh.ObjectHandles)
			{
				try
				{
					std::string name = _session->GetObjectStringProperty(id, mtp::ObjectProperty::ObjectFilename);
					_objectFormats[id] = (ObjectFormat)_session->GetObjectIntegerProperty(id, ObjectProperty::ObjectFormat);
					cache[name] = id;
				} catch(const std::exception &ex)
				{ }
			}
			return cache;
		}

		void Lookup (fuse_req_t req, fuse_ino_t parent, const char *name)
		{
			mtp::print("LOOKUP ", name, " IN ", parent);
			FuseEntry entry(req);
			if (parent != 1)
			{
				const ChildrenObjects & children = GetChildren(parent);
				auto it = children.find(name);
				if (it != children.end())
				{
					entry.ino = it->second;
					entry.SetFormat(GetObjectFormat(it->second));
					entry.Reply();
					return;
				}
			}
			else
			{
				auto sit = _storagesByName.find(name);
				if (sit != _storagesByName.end())
				{
					entry.ino = sit->second;
					entry.SetDirectoryMode();
					entry.Reply();
					return;
				}
			}
			entry.ReplyError(ENOENT);
		}

		void ReadDir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
		{
			FuseDirectory dir(req);
			if (ino == 1)
			{
				dir.Add(1, ".");
				dir.Add(1, "..");
				for(auto it : _storages)
				{
					dir.Add(it.first, it.second);
				}
				dir.Reply(off, size);
				return;
			}
			else
			{
				const ChildrenObjects & cache = GetChildren(ino);
				dir.Add(ino, ".");
				dir.Add(1, "..");
				for(auto it : cache)
				{
					dir.Add(it.second, it.first);
				}
				dir.Reply(off, size);
				return;
			}
			fuse_reply_err(req, ENOTDIR);
		}

		void Read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
		{
			mtp::ByteArray data = _session->GetPartialObject(ino, off, size);
			FUSE_CALL(fuse_reply_buf(req, static_cast<char *>(static_cast<void *>(data.data())), data.size()));
		}

		void GetAttr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
		{
			FuseEntry entry(req);
			entry.ino = ino;
			if (ino == 1)
			{
				entry.SetDirectoryMode();
				entry.ReplyAttr();
				return;
			}
			else
			{
				auto it = _storages.find((mtp::u32)ino);
				if (it != _storages.end())
				{
					entry.SetDirectoryMode();
					entry.ReplyAttr();
					return;
				}

				try {
					entry.SetFormat(GetObjectFormat(ino));
					entry.attr.st_size = _session->GetObjectIntegerProperty(ino, mtp::ObjectProperty::ObjectSize);
					entry.ReplyAttr();
					return;
				}
				catch(const std::exception &ex)
				{ }
			}
			entry.ReplyError(ENOENT);
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
		{ fprintf(stderr, #__VA_ARGS__ " failed: %s\n", ex.what()); fuse_reply_err(req, -EIO); } \
	} while(false)

	void Init (void *userdata, struct fuse_conn_info *conn)
	{ try { g_wrapper->Init(userdata, conn); } catch (const std::exception &ex) { mtp::error("init failed:", ex.what()); } }

	void Lookup (fuse_req_t req, fuse_ino_t parent, const char *name)
	{ WRAP_EX(g_wrapper->Lookup(req, parent, name)); }

	void ReadDir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->ReadDir(req, ino, size, off, fi)); }

	void GetAttr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->GetAttr(req, ino, fi)); }

	void Read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
	{ WRAP_EX(g_wrapper->Read(req, ino, size, off, fi)); }
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
//	ops.open		= &Open;
//	ops.create		= &Create;
	ops.read		= &Read;
//	ops.write		= &Write;
//	ops.mkdir		= &MakeDir;
//	ops.rmdir		= &RemoveDir;
//	ops.unlink		= &Unlink;
//	ops.truncate	= &Truncate;
//	ops.statfs		= &StatFS;
//	ops.chmod		= &ChangeMode;

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

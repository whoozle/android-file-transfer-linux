/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2017  Vladimir Menshakov

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

#include <cli/Session.h>
#include <cli/CommandLine.h>
#include <cli/PosixStreams.h>
#include <cli/ProgressBar.h>
#include <cli/Tokenizer.h>

#include <mtp/make_function.h>
#include <mtp/ptp/ByteArrayObjectStream.h>
#include <mtp/ptp/ObjectPropertyListParser.h>
#include <mtp/log.h>
#include <mtp/version.h>

#include <sstream>

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <set>

namespace
{
	bool BeginsWith(const std::string &str, const std::string &prefix)
	{
		if (prefix.size() > str.size())
			return false;
		const char *a = str.c_str();
		const char *b = prefix.c_str();
		return strncasecmp(a, b, prefix.size()) == 0;
	}
}


namespace cli
{

	Session::Session(const mtp::DevicePtr &device, bool showPrompt):
		_device(device),
		_session(_device->OpenSession(1)),
		_gdi(_session->GetDeviceInfo()),
		_cs(mtp::Session::AllStorages),
		_cd(mtp::Session::Root),
		_running(true),
		_interactive(isatty(STDOUT_FILENO)),
		_showPrompt(showPrompt),
		_terminalWidth(80)
	{
		using namespace mtp;
		using namespace std::placeholders;
		{
			const char *cols = getenv("COLUMNS");
			_terminalWidth = cols? atoi(cols): 80;
#ifdef TIOCGSIZE
			struct ttysize ts;
			ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
			_terminalWidth = ts.ts_cols;
#elif defined(TIOCGWINSZ)
			struct winsize ts;
			ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
			_terminalWidth = ts.ws_col;
#endif
		}

		AddCommand("help", "shows this help",
			make_function([this]() -> void { Help(); }));

		AddCommand("ls", "lists current directory",
			make_function([this]() -> void { List(false, false); }));
		AddCommand("ls", "<path> lists objects in <path>",
			make_function([this](const Path &path) -> void { List(path, false, false); }));

		AddCommand("ls-r", "lists current directory [recursive]",
			make_function([this]() -> void { List(false, true); }));
		AddCommand("ls-r", "<path> lists objects in <path> [recursive]",
			make_function([this](const Path &path) -> void { List(path, false, true); }));

		AddCommand("lsext", "lists current directory [extended info]",
			make_function([this]() -> void { List(true, false); }));
		AddCommand("lsext", "<path> lists objects in <path> [extended info]",
			make_function([this](const Path &path) -> void { List(path, true, false); }));

		AddCommand("lsext-r", "lists current directory [extended info, recursive]",
			make_function([this]() -> void { List(true, true); }));
		AddCommand("lsext-r", "<path> lists objects in <path> [extended info, recursive]",
			make_function([this](const Path &path) -> void { List(path, true, true); }));

		AddCommand("put", "<file> uploads file",
			make_function([this](const LocalPath &path) -> void { Put(path); }));

		AddCommand("put", "put <file> <dir> uploads file to directory",
			make_function([this](const LocalPath &path, const Path &dst) -> void { Put(path, dst); }));

		AddCommand("get", "<file> downloads file",
			make_function([this](const Path &path) -> void { Get(path); }));
		AddCommand("get", "<file> <dst> downloads file to <dst>",
			make_function([this](const Path &path, const LocalPath &dst) -> void { Get(dst, path); }));
		AddCommand("cat", "<file> outputs file",
			make_function([this](const Path &path) -> void { Cat(path); }));

		AddCommand("quit", "quits program",
			make_function([this]() -> void { Quit(); }));
		AddCommand("exit", "exits program",
			make_function([this]() -> void { Quit(); }));

		AddCommand("cd", "<path> change directory to <path>",
			make_function([this](const Path &path) -> void { ChangeDirectory(path); }));
		AddCommand("storage", "<storage-name>",
			make_function([this](const StoragePath &path) -> void { ChangeStorage(path); }));
		AddCommand("pwd", "resolved current object directory",
			make_function([this]() -> void { CurrentDirectory(); }));
		AddCommand("rm", "<path> removes object (WARNING: RECURSIVE, be careful!)",
			make_function([this](const LocalPath &path) -> void { Delete(path); }));
		AddCommand("mkdir", "<path> makes directory",
			make_function([this](const Path &path) -> void { MakeDirectory(path); }));
		AddCommand("mkpath", "<path> create directory structure specified in path",
			make_function([this](const Path &path) -> void { MakePath(path); }));
		AddCommand("type", "<path> shows type of file (recognized by libmagic/extension)",
			make_function([this](const LocalPath &path) -> void { ShowType(path); }));

		AddCommand("storage-list", "shows available MTP storages",
			make_function([this]() -> void { ListStorages(); }));
		AddCommand("properties", "<path> lists properties for <path>",
			make_function([this](const Path &path) -> void { ListProperties(path); }));
		AddCommand("device-properties", "shows device's MTP properties",
			make_function([this]() -> void { ListDeviceProperties(); }));

		AddCommand("test-property-list", "test GetObjectPropList on given object",
			make_function([this](const Path &path) -> void { TestObjectPropertyList(path); }));
#if 0
		auto test = [](const std::string &input)
		{
			Tokens tokens;
			Tokenizer(input, tokens);
			print(input);
			for(auto t : tokens)
				print("\t", t);
		};
		AddCommand("test-lexer", "tests lexer",
			make_function([&test]() -> void
			{
				test("a\\ b\\ c d");
				test("\"a b c\" d");
				test("\"\\\"\"");
			}));
#endif
	}

	char ** Session::CompletionCallback(const char *text, int start, int end)
	{
		Tokens tokens;
		Tokenizer(CommandLine::Get().GetLineBuffer(), tokens);
		if (tokens.size() < 2) //0 or 1
		{
			std::string command = !tokens.empty()? tokens.back(): std::string();
			char **comp = static_cast<char **>(calloc(sizeof(char *), _commands.size() + 1));
			auto it = _commands.begin();
			size_t i = 0, n = _commands.size();
			for(; n--; ++it)
			{
				if (end != 0 && !BeginsWith(it->first, command))
					continue;

				comp[i++] = strdup(it->first.c_str());
			}
			if (i == 0) //readline silently dereference matches[0]
			{
				free(comp);
				comp = NULL;
			};
			return comp;
		}
		else
		{
			//completion
			const std::string &commandName = tokens.front();
			auto b = _commands.lower_bound(commandName);
			auto e = _commands.upper_bound(commandName);
			if (b == e)
				return NULL;

			size_t idx = tokens.size() - 2;

			decltype(b) i;
			for(i = b; i != e; ++i)
			{
				if (idx < i->second->GetArgumentCount())
					break;
			}
			if (i == e)
				return NULL;

			//mtp::print("COMPLETING ", commandName, " ", idx, ":", commandName, " with ", i->second->GetArgumentCount(), " args");
			ICommandPtr command = i->second;
			std::list<std::string> matches;
			CompletionContext ctx(*this, idx, text, matches);
			command->Complete(ctx);
			if (ctx.Result.empty())
				return NULL;

			char **comp = static_cast<char **>(calloc(sizeof(char *), ctx.Result.size() + 1));
			size_t dst = 0;
			for(auto i : ctx.Result)
				comp[dst++] = strdup(i.c_str());
			return comp;
		}
		return NULL;
	}

	void Session::ProcessCommand(const std::string &input)
	{
		Tokens tokens;
		Tokenizer(input, tokens);
		if (!tokens.empty())
			ProcessCommand(std::move(tokens));
	}

	void Session::ProcessCommand(Tokens && tokens_)
	{
		Tokens tokens(tokens_);
		if (tokens.empty())
			throw std::runtime_error("no token passed to ProcessCommand");

		std::string cmdName = tokens.front();
		tokens.pop_front();
		auto cmd = _commands.find(cmdName);
		if (cmd == _commands.end())
			throw std::runtime_error("invalid command " + cmdName);

		cmd->second->Execute(tokens);
	}

	void Session::UpdatePrompt()
	{
		if (_showPrompt)
			_prompt = _gdi.Manufacturer + " " + _gdi.Model + (!_csName.empty()? ":" + _csName: std::string()) + "> ";
		else
			_prompt.clear();
	}

	void Session::InteractiveInput()
	{
		using namespace mtp;
		if (_interactive && _showPrompt)
		{
			print("android file transfer for linux version ", GetVersion());
			print(_gdi.Manufacturer, " ", _gdi.Model, " ", _gdi.DeviceVersion);
			print("extensions: ", _gdi.VendorExtensionDesc);
			//print(_gdi.SerialNumber); //non-secure
			std::stringstream ss;
			ss << "supported op codes: ";
			for(OperationCode code : _gdi.OperationsSupported)
			{
				ss << hex(code, 4) << " ";
			}
			ss << "\n";
			ss << "supported properties: ";
			for(u16 code : _gdi.DevicePropertiesSupported)
			{
				ss << hex(code, 4) << " ";
			}
			ss << "\n";
			debug(ss.str());
		}

		cli::CommandLine::Get().SetCallback([this](const char *text, int start, int end) -> char ** { return CompletionCallback(text, start, end); });
		UpdatePrompt();

		std::string input;
		while (_showPrompt? cli::CommandLine::Get().ReadLine(_prompt, input): cli::CommandLine::Get().ReadRawLine(input))
		{
			try
			{
				ProcessCommand(input);
				if (!_running) //do not put newline
					return;
			}
			catch (const mtp::InvalidResponseException &ex)
			{
				mtp::error("error: ", ex.what());
				if (ex.Type == mtp::ResponseType::InvalidStorageID)
					error("\033[1mYour device might be locked or in usb-charging mode, please unlock it and put it in MTP or PTP mode\033[0m\n");
			}
			catch(const std::exception &ex)
			{ error("error: ", ex.what()); }
		}
		if (_showPrompt)
			print("");
	}

	mtp::ObjectId Session::ResolveObjectChild(mtp::ObjectId parent, const std::string &entity)
	{
		//fixme: replace with prop list!
		auto objectList = _session->GetObjectHandles(_cs, mtp::ObjectFormat::Any, parent);
		for(auto object : objectList.ObjectHandles)
		{
			std::string name = _session->GetObjectStringProperty(object, mtp::ObjectProperty::ObjectFilename);
			if (name == entity)
			{
				return object;
			}
		}
		throw std::runtime_error("could not find " + entity + " in path");
	}

	mtp::ObjectId Session::Resolve(const Path &path, bool create)
	{
		mtp::ObjectId id = BeginsWith(path, "/")? mtp::Session::Root: _cd;
		for(size_t p = 0; p < path.size(); )
		{
			size_t next = path.find('/', p);
			if (next == path.npos)
				next = path.size();

			std::string entity(path.substr(p, next - p));
			if (entity.empty() || entity == ".")
			{ }
			else
			if (entity == "..")
			{
				id = _session->GetObjectParent(id);
				if (id == mtp::Session::Device)
					id = mtp::Session::Root;
			}
			else
			{
				try
				{
					id = ResolveObjectChild(id, entity);
				}
				catch(const std::exception &ex)
				{
					if (!create)
						throw;
					id = MakeDirectory(id, entity);
				}
			}
			p = next + 1;
		}
		return id;
	}

	std::string Session::GetFilename(const std::string &path)
	{
		size_t pos = path.rfind('/');
		return (pos == path.npos)? path: path.substr(pos + 1);
	}

	std::string Session::GetDirname(const std::string &path)
	{
		size_t pos = path.rfind('/');
		return (pos == path.npos)? std::string(): path.substr(0, pos);
	}

	std::string Session::FormatTime(const std::string &timespec)
	{
		if (timespec.size() != 15 || timespec[8] != 'T')
			return timespec;

		return
			timespec.substr(0, 4) + "-" +
			timespec.substr(4, 2) + "-" +
			timespec.substr(6, 2) + " " +
			timespec.substr(9, 2) + ":" +
			timespec.substr(11, 2) + ":" +
			timespec.substr(13, 2);
	}

	mtp::ObjectId Session::ResolvePath(const std::string &path, std::string &file)
	{
		size_t pos = path.rfind('/');
		if (pos == path.npos)
		{
			file = path;
			return _cd;
		}
		else
		{
			file = path.substr(pos + 1);
			return Resolve(path.substr(0, pos));
		}
	}

	void Session::CurrentDirectory()
	{
		std::string path;
		mtp::ObjectId id = _cd;
		while(id != mtp::Session::Device && id != mtp::Session::Root)
		{
			std::string filename = _session->GetObjectStringProperty(id, mtp::ObjectProperty::ObjectFilename);
			path = filename + "/" + path;
			id = _session->GetObjectParent(id);
			if (id == mtp::Session::Device)
				break;
		}
		path = "/" + path;
		mtp::print(path);
	}


	void Session::List(mtp::ObjectId parent, bool extended, bool recursive, const std::string &prefix)
	{
		using namespace mtp;
		if (!extended && !recursive && _cs == mtp::Session::AllStorages && _session->GetObjectPropertyListSupported())
		{
			ByteArray data = _session->GetObjectPropertyList(parent, ObjectFormat::Any, ObjectProperty::ObjectFilename, 0, 1);
			ObjectPropertyListParser<std::string> parser;
			//HexDump("list", data, true);
			parser.Parse(data, [&prefix](ObjectId objectId, ObjectProperty property, const std::string &name)
			{
				print(std::left, width(objectId, 10), " ", prefix + name);
			});
		}
		else
		{
			msg::ObjectHandles handles = _session->GetObjectHandles(_cs, mtp::ObjectFormat::Any, parent);

			for(auto objectId : handles.ObjectHandles)
			{
				try
				{
					msg::ObjectInfo info = _session->GetObjectInfo(objectId);
					if (extended)
						print(
							std::left,
							width(objectId, 10), " ",
							width(info.StorageId.Id, 10), " ",
							std::right,
							hex(info.ObjectFormat, 4), " ",
							width(info.ObjectCompressedSize, 10), " ",
							FormatTime(info.CaptureDate), " ",
							prefix + info.Filename, " "
						);
					else
						print(std::left, width(objectId, 10), " ", prefix + info.Filename);

					if (recursive && info.ObjectFormat == mtp::ObjectFormat::Association)
						List(objectId, extended, recursive, prefix + info.Filename + "/");
				}
				catch(const std::exception &ex)
				{
					mtp::error("error: ", ex.what());
				}
			}
		}
	}

	void Session::CompletePath(const Path &path, CompletionResult &result)
	{
		std::string filePrefix;
		mtp::ObjectId parent = ResolvePath(path, filePrefix);
		std::string dir = GetDirname(path);
		auto objectList = _session->GetObjectHandles(_cs, mtp::ObjectFormat::Any, parent);
		for(auto object : objectList.ObjectHandles)
		{
			std::string name = _session->GetObjectStringProperty(object, mtp::ObjectProperty::ObjectFilename);
			if (BeginsWith(name, filePrefix))
			{
				if (!dir.empty())
					name = dir + '/' + name;

				mtp::ObjectFormat format = (mtp::ObjectFormat)_session->GetObjectIntegerProperty(object, mtp::ObjectProperty::ObjectFormat);
				if (format == mtp::ObjectFormat::Association)
					name += '/';

				result.push_back(EscapePath(name));
			}
		}
	}

	void Session::CompleteStoragePath(const StoragePath &path, CompletionResult &result)
	{
		using namespace mtp;
		msg::StorageIDs list = _session->GetStorageIDs();
		for(size_t i = 0; i < list.StorageIDs.size(); ++i)
		{
			auto id = list.StorageIDs[i];
			msg::StorageInfo si = _session->GetStorageInfo(id);
			auto idStr = std::to_string(id.Id);
			if (BeginsWith(idStr, path))
				result.push_back(idStr);
			if (BeginsWith(si.VolumeLabel, path))
				result.push_back(EscapePath(si.VolumeLabel));
			if (BeginsWith(si.StorageDescription, path))
				result.push_back(EscapePath(si.StorageDescription));
		}
	}

	void Session::ListStorages()
	{
		using namespace mtp;
		msg::StorageIDs list = _session->GetStorageIDs();
		for(size_t i = 0; i < list.StorageIDs.size(); ++i)
		{
			msg::StorageInfo si = _session->GetStorageInfo(list.StorageIDs[i]);
			print(
				std::left, width(list.StorageIDs[i], 8),
				" volume: ", si.VolumeLabel,
				", description: ", si.StorageDescription);
		}
	}

	void Session::ChangeStorage(const StoragePath &path)
	{
		using namespace mtp;
		if (path == "All" || path == "all" || path == "*")
		{
			_cs = mtp::Session::AllStorages;
			_csName.clear();
			UpdatePrompt();
			return;
		}
		msg::StorageIDs list = _session->GetStorageIDs();
		for(size_t i = 0; i < list.StorageIDs.size(); ++i)
		{
			auto id = list.StorageIDs[i];
			msg::StorageInfo si = _session->GetStorageInfo(id);
			auto idStr = std::to_string(id.Id);
			if (idStr == path || si.StorageDescription == path || si.VolumeLabel == path)
			{
				_cs = id;
				_csName = si.GetName();
				print("selected storage ", _cs.Id, " ", si.VolumeLabel, " ", si.StorageDescription);
				UpdatePrompt();
				return;
			}
		}
		throw std::runtime_error("storage " + path + " could not be found");
	}

	void Session::Help()
	{
		using namespace mtp;
		print("Available commands are:");
		for(auto i : _commands)
		{
			print("\t", std::left, width(i.first, 20), i.second->GetHelpString());
		}
	}

	void Session::Get(const LocalPath &dst, mtp::ObjectId srcId)
	{
		mtp::ObjectFormat format = static_cast<mtp::ObjectFormat>(_session->GetObjectIntegerProperty(srcId, mtp::ObjectProperty::ObjectFormat));
		if (format == mtp::ObjectFormat::Association)
		{
			mkdir(dst.c_str(), 0700);
			auto obj = _session->GetObjectHandles(_cs, mtp::ObjectFormat::Any, srcId);
			for(auto id : obj.ObjectHandles)
			{
				auto info = _session->GetObjectInfo(id);
				LocalPath dstFile = dst + "/" + info.Filename;
				Get(dstFile, id);
			}
		}
		else
		{
			auto stream = std::make_shared<ObjectOutputStream>(dst);
			if (IsInteractive())
			{
				mtp::u64 size = _session->GetObjectIntegerProperty(srcId, mtp::ObjectProperty::ObjectSize);
				stream->SetTotal(size);
				if (IsInteractive())
					try { stream->SetProgressReporter(ProgressBar(dst, _terminalWidth / 3, _terminalWidth)); } catch(const std::exception &ex) { }
			}
			_session->GetObject(srcId, stream);
		}
	}

	void Session::Get(mtp::ObjectId srcId)
	{
		auto info = _session->GetObjectInfo(srcId);
		Get(LocalPath(info.Filename), srcId);
	}

	void Session::Cat(const Path &path)
	{
		mtp::ByteArrayObjectOutputStreamPtr stream(new mtp::ByteArrayObjectOutputStream);
		_session->GetObject(Resolve(path), stream);
		const mtp::ByteArray & data = stream->GetData();
		std::string text(data.begin(), data.end());
		fputs(text.c_str(), stdout);
		if (text.empty() || text[text.size() - 1] == '\n')
			fputc('\n', stdout);
	}

	void Session::Put(mtp::ObjectId parentId, const LocalPath &src)
	{
		using namespace mtp;
		struct stat st = {};
		if (stat(src.c_str(), &st))
			throw std::runtime_error(std::string("stat failed: ") + strerror(errno));

		if (S_ISDIR(st.st_mode))
		{
			std::string name = GetFilename(src.back() == '/'? src.substr(0, src.size() - 1): static_cast<const std::string &>(src));
			try
			{ parentId = ResolveObjectChild(parentId, name); }
			catch(const std::exception &ex)
			{ parentId = MakeDirectory(parentId, name); }

			DIR *dir = opendir(src.c_str());
			if (!dir)
			{
				perror("opendir");
				return;
			}
			while(true)
			{
				dirent *result = readdir(dir);
				if (!result)
					break;

				if (strcmp(result->d_name, ".") == 0 || strcmp(result->d_name, "..") == 0)
					continue;

				std::string fname = result->d_name;
				Put(parentId, src + "/" + fname);
			}
			closedir(dir);
		}
		else
		{
			std::string filename = GetFilename(src);
			try
			{
				mtp::ObjectId objectId = ResolveObjectChild(parentId, filename);
				_session->DeleteObject(objectId);
			}
			catch(const std::exception &ex)
			{ }

			auto stream = std::make_shared<ObjectInputStream>(src);
			stream->SetTotal(stream->GetSize());

			msg::ObjectInfo oi;
			oi.Filename = filename;
			oi.ObjectFormat = ObjectFormatFromFilename(src);
			oi.SetSize(stream->GetSize());

			if (IsInteractive())
				try { stream->SetProgressReporter(ProgressBar(src, _terminalWidth / 3, _terminalWidth)); } catch(const std::exception &ex) { }

			_session->SendObjectInfo(oi, GetUploadStorageId(), parentId);
			_session->SendObject(stream);
		}
	}

	void Session::Put(const LocalPath &src, const Path &dst)
	{
		Put(Resolve(dst, true), src); //fixme: fix put <file> <path/file> case
	}

	mtp::ObjectId Session::MakeDirectory(mtp::ObjectId parentId, const std::string & name)
	{
		using namespace mtp;
		msg::ObjectInfo oi;
		oi.Filename = name;
		oi.ObjectFormat = ObjectFormat::Association;
		auto noi = _session->SendObjectInfo(oi, GetUploadStorageId(), parentId);
		return noi.ObjectId;
	}

	void Session::ShowType(const LocalPath &src)
	{
		mtp::ObjectFormat format = mtp::ObjectFormatFromFilename(src);
		print("mtp object format = ", hex(format, 4));
	}

	void Session::ListProperties(mtp::ObjectId id)
	{
		auto ops = _session->GetObjectPropertiesSupported(id);
		std::stringstream ss;
		ss << "properties supported: ";
		for(mtp::ObjectProperty prop: ops.ObjectPropertyCodes)
		{
			ss << mtp::hex(prop, 4) << " ";
		}
		ss << "\n";
		mtp::print(ss.str());
	}

	void Session::ListDeviceProperties()
	{
		using namespace mtp;
		for(u16 code : _gdi.DevicePropertiesSupported)
		{
			print("property code: ", hex(code, 4));
			ByteArray data = _session->GetDeviceProperty((mtp::DeviceProperty)code);
			HexDump("value", data, true);
		}
	}

	namespace
	{
		template<typename>
		struct DummyPropertyListParser
		{
			static mtp::ByteArray Parse(mtp::InputStream &stream, mtp::DataTypeCode dataType)
			{
				switch(dataType)
				{
#define CASE(BITS) \
					case mtp::DataTypeCode::Uint##BITS: \
					case mtp::DataTypeCode::Int##BITS: \
						stream.Skip(BITS / 8); break
					CASE(8); CASE(16); CASE(32); CASE(64); CASE(128);
#undef CASE
					case mtp::DataTypeCode::String:
						stream.ReadString(); break;
					default:
						throw std::runtime_error("got invalid data type code");
				}
				return mtp::ByteArray();
			}
		};
	}

	void Session::GetObjectPropertyList(mtp::ObjectId parent, const std::set<mtp::ObjectId> &originalObjectList, const mtp::ObjectProperty property)
	{
		using namespace mtp;
		print("testing property 0x", hex(property, 4), "...");

		std::set<ObjectId> objectList;
		ByteArray data = _session->GetObjectPropertyList(parent, ObjectFormat::Any, property, 0, 1);
		print("got ", data.size(), " bytes of reply");
		HexDump("property list", data);
		ObjectPropertyListParser<ByteArray, DummyPropertyListParser> parser;

		bool ok = true;

		parser.Parse(data, [this, &objectList, property, &ok](mtp::ObjectId objectId, ObjectProperty p, const ByteArray & ) {
			if ((p == property || property == ObjectProperty::All))
				objectList.insert(objectId);
			else
			{
				print("extra property 0x", hex(p, 4), " returned for object ", objectId.Id, ", while querying property list 0x", mtp::hex(property, 4));
				ok = false;
			}
		});

		std::set<ObjectId> extraData;
		std::set_difference(objectList.begin(), objectList.end(), originalObjectList.begin(), originalObjectList.end(), std::inserter(extraData, extraData.end()));

		if (!extraData.empty())
		{
			print("inconsistent GetObjectPropertyList for property 0x", mtp::hex(property, 4));
			for(auto objectId : extraData)
			{
				print("missing 0x", mtp::hex(property, 4), " for object ", objectId);
				ok = false;
			}
		}
		print("getting object property list of type 0x", hex(property, 4), " ", ok? "PASSED": "FAILED");
	}


	void Session::TestObjectPropertyList(const Path &path)
	{
		using namespace mtp;
		ObjectId id = Resolve(path);
		msg::ObjectHandles oh = _session->GetObjectHandles(_cs, ObjectFormat::Any, id);

		std::set<ObjectId> objectList;
		for(auto id : oh.ObjectHandles)
			objectList.insert(id);

		print("GetObjectHandles ", id, " returns ", oh.ObjectHandles.size(), " objects, ", objectList.size(), " unique");
		GetObjectPropertyList(id, objectList, ObjectProperty::ObjectFilename);
		GetObjectPropertyList(id, objectList, ObjectProperty::ObjectFormat);
		GetObjectPropertyList(id, objectList, ObjectProperty::ObjectSize);
		GetObjectPropertyList(id, objectList, ObjectProperty::DateModified);
		GetObjectPropertyList(id, objectList, ObjectProperty::DateAdded);
		GetObjectPropertyList(id, objectList, ObjectProperty::All);
	}

}

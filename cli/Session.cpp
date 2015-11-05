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

#include <cli/Session.h>
#include <cli/CommandLine.h>
#include <cli/PosixStreams.h>
#include <cli/ProgressBar.h>
#include <cli/Tokenizer.h>

#include <mtp/make_function.h>
#include <mtp/ptp/ByteArrayObjectStream.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

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

	Session::Session(const mtp::DevicePtr &device):
		_device(device),
		_session(_device->OpenSession(1)),
		_gdi(_session->GetDeviceInfo()),
		_cs(mtp::Session::AllStorages),
		_cd(mtp::Session::Root),
		_running(true),
		_interactive(isatty(STDOUT_FILENO))
	{
		using namespace mtp;
		using namespace std::placeholders;
		{
			const char *cols = getenv("COLUMNS");
			_terminalWidth = cols? atoi(cols): 80;
		}

		printf("%s\n", _gdi.VendorExtensionDesc.c_str());
		printf("%s ", _gdi.Manufacturer.c_str());
		printf("%s ", _gdi.Model.c_str());
		printf("%s ", _gdi.DeviceVersion.c_str());
		//printf("%s", _gdi.SerialNumber.c_str());
		printf("\n");
		printf("supported op codes: ");
		for(OperationCode code : _gdi.OperationsSupported)
		{
			printf("%04x ", (unsigned)code);
		}
		printf("\n");
		printf("supported properties: ");
		for(u16 code : _gdi.DevicePropertiesSupported)
		{
			printf("%04x ", (unsigned)code);
		}
		printf("\n");

		AddCommand("help", "shows this help",
			make_function([this]() -> void { Help(); }));

		AddCommand("ls", "lists current directory",
			make_function([this]() -> void { List(); }));
		AddCommand("ls", "<path> lists objects in <path>",
			make_function([this](const Path &path) -> void { List(path); }));

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
		AddCommand("pwd", "resolved current object directory",
			make_function([this]() -> void { CurrentDirectory(); }));
		AddCommand("rm", "<path> removes object (WARNING: RECURSIVE, be careful!)",
			make_function([this](const LocalPath &path) -> void { Delete(path); }));
		AddCommand("mkdir", "<path> makes directory",
			make_function([this](const Path &path) -> void { MakeDirectory(path); }));
		AddCommand("type", "<path> shows type of file (recognized by libmagic/extension)",
			make_function([this](const LocalPath &path) -> void { ShowType(path); }));

		AddCommand("storage-list", "shows available MTP storages",
			make_function([this]() -> void { ListStorages(); }));
		AddCommand("device-properties", "shows device's MTP properties",
			make_function([this]() -> void { ListDeviceProperties(); }));
#if 0
		auto test = [](const std::string &input)
		{
			Tokens tokens;
			Tokenizer(input, tokens);
			printf("%s:\n", input.c_str());
			for(auto t : tokens)
				printf("\t%s\n", t.c_str());
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
			//printf("COMPLETING %s:%u with %u args\n", commandName.c_str(), idx, i->second->GetArgumentCount());
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

	void Session::InteractiveInput()
	{
		using namespace mtp;
		std::string prompt(_gdi.Manufacturer + " " + _gdi.Model + "> "), input;
		cli::CommandLine::Get().SetCallback([this](const char *text, int start, int end) -> char ** { return CompletionCallback(text, start, end); });

		while (cli::CommandLine::Get().ReadLine(prompt, input))
		{
			try
			{
				ProcessCommand(input);
				if (!_running) //do not put newline
					return;
			}
			catch (const mtp::InvalidResponseException &ex)
			{
				printf("error: %s\n", ex.what());
				if (ex.Type == mtp::ResponseType::InvalidStorageID)
					printf("\033[1mYour device might be locked or in usb-charging mode, please unlock it and put it in MTP or PTP mode\033[0m\n");
			}
			catch(const std::exception &ex)
			{ printf("error: %s\n", ex.what()); }
		}
		printf("\n");
	}

	mtp::u32 Session::ResolveObjectChild(mtp::u32 parent, const std::string &entity)
	{
		mtp::u32 id = 0;
		auto objectList = _session->GetObjectHandles(_cs, mtp::Session::AllFormats, parent);
		for(auto object : objectList.ObjectHandles)
		{
			std::string name = _session->GetObjectStringProperty(object, mtp::ObjectProperty::ObjectFilename);
			if (name == entity)
			{
				id = object;
				break;
			}
		}
		return id;
	}

	mtp::u32 Session::Resolve(const Path &path)
	{
		mtp::u32 id = BeginsWith(path, "/")? mtp::Session::Root: _cd;
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
				id = _session->GetObjectIntegerProperty(id, mtp::ObjectProperty::ParentObject);
				if (id == 0)
					id = mtp::Session::Root;
			}
			else
			{
				id = ResolveObjectChild(id, entity);
				if (!id)
					throw std::runtime_error("could not find " + entity + " in path " + path.substr(0, p));
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

	mtp::u32 Session::ResolvePath(const std::string &path, std::string &file)
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
		mtp::u32 id = _cd;
		while(id && id != mtp::Session::Root)
		{
			std::string filename = _session->GetObjectStringProperty(id, mtp::ObjectProperty::ObjectFilename);
			path = filename + "/" + path;
			id = _session->GetObjectIntegerProperty(id, mtp::ObjectProperty::ParentObject);
			if (id == 0)
				break;
		}
		path = "/" + path;
		printf("%s\n", path.c_str());
	}


	void Session::List(mtp::u32 parent)
	{
		using namespace mtp;
		msg::ObjectHandles handles = _session->GetObjectHandles(_cs, mtp::Session::AllFormats, parent);

		for(u32 objectId : handles.ObjectHandles)
		{
			try
			{
				msg::ObjectInfo info = _session->GetObjectInfo(objectId);
				printf("%-10u %04hx %10u %s %ux%u, %s\n", objectId, (unsigned)info.ObjectFormat, info.ObjectCompressedSize, info.Filename.c_str(), info.ImagePixWidth, info.ImagePixHeight, info.CaptureDate.c_str());
			}
			catch(const std::exception &ex)
			{
				printf("error: %s\n", ex.what());
			}
		}
	}

	void Session::CompletePath(const Path &path, CompletionResult &result)
	{
		std::string filePrefix;
		mtp::u32 parent = ResolvePath(path, filePrefix);
		std::string dir = GetDirname(path);
		auto objectList = _session->GetObjectHandles(_cs, mtp::Session::AllFormats, parent);
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

				if (name.find(' ') != name.npos)
					result.push_back('"' + name +'"');
				else
					result.push_back(name);
			}
		}
	}

	void Session::ListStorages()
	{
		using namespace mtp;
		msg::StorageIDs list = _session->GetStorageIDs();
		for(size_t i = 0; i < list.StorageIDs.size(); ++i)
		{
			msg::StorageInfo si = _session->GetStorageInfo(list.StorageIDs[i]);
			printf("%08d volume: %s, description: %s\n", list.StorageIDs[i], si.VolumeLabel.c_str(), si.StorageDescription.c_str());
		}
	}

	void Session::Help()
	{
		printf("Available commands are:\n");
		for(auto i : _commands)
		{
			printf("\t%-20s %s\n", i.first.c_str(), i.second->GetHelpString().c_str());
		}
	}

	void Session::Get(const LocalPath &dst, mtp::u32 srcId)
	{
		mtp::ObjectFormat format = static_cast<mtp::ObjectFormat>(_session->GetObjectIntegerProperty(srcId, mtp::ObjectProperty::ObjectFormat));
		if (format == mtp::ObjectFormat::Association)
		{
			mkdir(dst.c_str(), 0700);
			auto obj = _session->GetObjectHandles(_cs, mtp::Session::AllFormats, srcId);
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

	void Session::Get(mtp::u32 srcId)
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

	void Session::Put(mtp::u32 parentId, const std::string &dst, const LocalPath &src)
	{
		using namespace mtp;
		struct stat st = {};
		if (stat(src.c_str(), &st))
			throw std::runtime_error(std::string("stat failed: ") + strerror(errno));

		if (S_ISDIR(st.st_mode))
		{
			mtp::u32 dirId = 0;

			try
			{ dirId = _session->CreateDirectory(GetFilename(dst), parentId).ObjectId; }
			catch(const std::exception & ex)
			{ }

			if (!dirId)
				dirId = ResolveObjectChild(parentId, src);
			if (!dirId)
				throw std::runtime_error("cannot create/resolve directory '" + GetFilename(dst) + "'");

			DIR *dir = opendir(src.c_str());
			if (!dir)
			{
				perror("opendir");
				return;
			}
			dirent entry = {};
			while(true)
			{
				dirent *result = NULL;
				if (readdir_r(dir, &entry, &result) != 0)
					throw std::runtime_error(std::string("readdir failed: ") + strerror(errno));
				if (!result)
					break;

				if (strcmp(result->d_name, ".") == 0 || strcmp(result->d_name, "..") == 0)
					continue;

				std::string fname = result->d_name;
				Put(dirId, dst + "/" + fname, src + "/" + fname);
			}
			closedir(dir);
		}
		else
		{
			auto stream = std::make_shared<ObjectInputStream>(src);
			stream->SetTotal(stream->GetSize());

			msg::ObjectInfo oi;
			oi.Filename = GetFilename(dst);
			oi.ObjectFormat = ObjectFormatFromFilename(src);
			oi.SetSize(stream->GetSize());

			if (IsInteractive())
				try { stream->SetProgressReporter(ProgressBar(dst, _terminalWidth / 3, _terminalWidth)); } catch(const std::exception &ex) { }

			_session->SendObjectInfo(oi, 0, parentId);
			_session->SendObject(stream);
		}
	}

	void Session::MakeDirectory(mtp::u32 parentId, const std::string & name)
	{
		using namespace mtp;
		msg::ObjectInfo oi;
		oi.Filename = name;
		oi.ObjectFormat = ObjectFormat::Association;
		_session->SendObjectInfo(oi, 0, parentId);
	}

	void Session::ShowType(const LocalPath &src)
	{
		mtp::ObjectFormat format = mtp::ObjectFormatFromFilename(src);
		printf("mtp object format = %04x\n", (unsigned)format);
	}

	void Session::ListProperties(mtp::u32 id)
	{
		auto ops = _session->GetObjectPropsSupported(id);
		printf("properties supported: ");
		for(mtp::u16 prop: ops.ObjectPropCodes)
		{
			printf("%02x ", prop);
		}
		printf("\n");
	}

	void Session::ListDeviceProperties()
	{
		using namespace mtp;
		for(u16 code : _gdi.DevicePropertiesSupported)
		{
			if ((code & 0xff00) != 0x5000 )
				continue;
			printf("property code: %04x\n", (unsigned)code);
			ByteArray data = _session->GetDeviceProperty((mtp::DeviceProperty)code);
			HexDump("value", data);
		}
	}

}

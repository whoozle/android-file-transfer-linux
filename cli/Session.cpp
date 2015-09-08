#include <cli/Session.h>
#include <cli/CommandLine.h>
#include <cli/PosixStreams.h>
#include <cli/Tokenizer.h>

#include <mtp/make_function.h>

#include <stdio.h>

namespace cli
{

	Session::Session(const mtp::DevicePtr &device):
		_device(device),
		_session(_device->OpenSession(1)),
		_gdi(_session->GetDeviceInfo()),
		_cd(mtp::Session::Root),
		_running(true)
	{
		using namespace mtp;
		using namespace std::placeholders;

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

		AddCommand("ls",			make_function([this]() -> void { List(); }));
		AddCommand("list",			make_function([this]() -> void { List(); }));
		AddCommand("list", 			make_function([this](mtp::u32 parent) -> void { List(parent); }));
		AddCommand("quit", 			make_function([this]() -> void { Quit(); }));
		AddCommand("exit", 			make_function([this]() -> void { Quit(); }));
		AddCommand("storages", make_function([this]() -> void { ListStorages(); }));
		AddCommand("device-properties", make_function([this]() -> void { ListDeviceProperties(); }));
		AddCommand("help",			make_function([this]() -> void { Help(); }));
	}

	char ** Session::CompletionCallback(const char *text, int start, int end)
	{
		if (start == 0)
		{
			char **comp = static_cast<char **>(calloc(sizeof(char *), _commands.size() + 1));
			auto it = _commands.begin();
			size_t i = 0, n = _commands.size();
			for(; n--; ++it)
			{
				if (end != 0 && it->first.compare(0, end, text) != 0)
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
		return NULL;
	}

	void Session::InteractiveInput()
	{
		using namespace mtp;
		std::string prompt(_gdi.Manufacturer + " " + _gdi.Model + ">"), input;
		cli::CommandLine::Get().SetCallback([this](const char *text, int start, int end) -> char ** { return CompletionCallback(text, start, end); });

		while (_running && cli::CommandLine::Get().ReadLine(prompt, input))
		{
			try
			{
				Tokens tokens;
				Tokenizer(input, tokens);
				if (tokens.empty())
					continue;

				std::string cmdName = tokens.front();
				tokens.pop_front();
				auto cmd = _commands.find(cmdName);
				if (cmd == _commands.end())
					throw std::runtime_error("invalid command " + cmdName);

				cmd->second->Execute(tokens);
			}
			catch(const std::exception &ex)
			{ printf("error: %s\n", ex.what()); }
		}
		printf("\n");
	}

	mtp::u32 Session::Resolve(const Path &path)
	{
		return mtp::Session::Root;
	}

	void Session::List(mtp::u32 parent)
	{
		using namespace mtp;
		msg::ObjectHandles handles = _session->GetObjectHandles(mtp::Session::AllStorages, mtp::Session::AllFormats, parent);

		for(u32 objectId : handles.ObjectHandles)
		{
			try
			{
				msg::ObjectInfo info = _session->GetObjectInfo(objectId);
				printf("%-10u %04hx %s %u %ux%u, %s\n", objectId, info.ObjectFormat, info.Filename.c_str(), info.ObjectCompressedSize, info.ImagePixWidth, info.ImagePixHeight, info.CaptureDate.c_str());
			}
			catch(const std::exception &ex)
			{
				printf("error: %s\n", ex.what());
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
			printf("\t%s\n", i.first.c_str());
		}
	}

	void Session::Get(const LocalPath &dst, mtp::u32 srcId)
	{
		_session->GetObject(srcId, std::make_shared<ObjectOutputStream>(dst));
	}

	void Session::Get(mtp::u32 srcId)
	{
		auto info = _session->GetObjectInfo(srcId);
		printf("filename = %s\n", info.Filename.c_str());
		Get(LocalPath(info.Filename), srcId);
	}

	void Session::Put(mtp::u32 parentId, const LocalPath &src)
	{
		using namespace mtp;
		msg::ObjectInfo oi;
		oi.Filename = src;
		oi.ObjectFormat = ObjectFormatFromFilename(src);

		std::shared_ptr<ObjectInputStream> objectInput(new ObjectInputStream(src));
		oi.SetSize(objectInput->GetSize());

		auto noi = _session->SendObjectInfo(oi, 0, parentId);
		printf("new object id = %u\n", noi.ObjectId);
		_session->SendObject(objectInput);
		printf("done\n");
	}

	void Session::MakeDirectory(mtp::u32 parentId, const std::string & name)
	{
		using namespace mtp;
		msg::ObjectInfo oi;
		oi.Filename = name;
		oi.ObjectFormat = ObjectFormat::Association;
		_session->SendObjectInfo(oi, 0, parentId);
	}

	void Session::Delete(mtp::u32 id)
	{
		_session->DeleteObject(id);
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

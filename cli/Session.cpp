#include <cli/Session.h>
#include <cli/CommandLine.h>
#include <cli/PosixStreams.h>

#include <stdio.h>

namespace cli
{
	Session::Session(const mtp::DevicePtr &device):
		_device(device),
		_session(_device->OpenSession(1)),
		_gdi(_session->GetDeviceInfo()),
		_cd(mtp::Session::Root)
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

		AddCommand("ls", std::bind(&Session::List, this));
		AddCommand("ls", std::bind(&Session::List, this, _1));
	}


	void Session::InteractiveInput()
	{
		std::string prompt(_gdi.Manufacturer + " " + _gdi.Model + ">"), input;

		while(cli::CommandLine::Get().ReadLine(prompt, input))
		{
		}
		printf("\n");
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

#if 0
	else if (command == "storages")
	{
		msg::StorageIDs list = session->GetStorageIDs();
		for(size_t i = 0; i < list.StorageIDs.size(); ++i)
		{
			msg::StorageInfo si = session->GetStorageInfo(list.StorageIDs[i]);
			printf("%08d volume: %s, description: %s\n", list.StorageIDs[i], si.VolumeLabel.c_str(), si.StorageDescription.c_str());
		}
	}
	else if (command == "get")
	{
		if (argc < 3)
			return 1;
		mtp::u32 objectId;
		if (sscanf(argv[2], "%u", &objectId) != 1)
			return 1;
		printf("object id = %u\n", objectId);
		msg::ObjectInfo info = session->GetObjectInfo(objectId);
		printf("filename = %s\n", info.Filename.c_str());

		session->GetObject(objectId, std::make_shared<ObjectOutputStream>(info.Filename));
	}
	else if (command == "set")
	{
		if (argc < 4)
			return 1;

		mtp::u32 parentObjectId;
		if (sscanf(argv[2], "%u", &parentObjectId) != 1)
			return 1;

		std::string filename(argv[3]);
		printf("uploading %s to %u\n", filename.c_str(), parentObjectId);

		msg::ObjectInfo oi;
		oi.Filename = filename;
		oi.ObjectFormat = ObjectFormatFromFilename(filename);

		std::shared_ptr<ObjectInputStream> objectInput(new ObjectInputStream(filename));
		oi.SetSize(objectInput->GetSize());

		Session::NewObjectInfo noi = session->SendObjectInfo(oi, 0, parentObjectId);
		printf("new object id = %u\n", noi.ObjectId);
		session->SendObject(objectInput);
		printf("done\n");
	}
	else if (command == "delete")
	{
		if (argc < 3)
			return 1;

		mtp::u32 objectId;
		if (sscanf(argv[2], "%u", &objectId) != 1)
			return 1;

		printf("object id = %u\n", objectId);
		session->DeleteObject(objectId);
	}
	else if (command == "mkdir")
	{
		if (argc < 3)
			return 1;

		mtp::u32 parentObjectId = mtp::Session::Root;
		if (argc > 3 && sscanf(argv[3], "%u", &parentObjectId) != 1)
			return 1;

		std::string filename = argv[2];
		msg::ObjectInfo oi;
		oi.Filename = filename;
		oi.ObjectFormat = ObjectFormat::Association;
		session->SendObjectInfo(oi, 0, parentObjectId);
	}
	else if (command == "properties")
	{
		if (argc < 3)
			return 1;

		mtp::u32 objectId;
		if (sscanf(argv[2], "%u", &objectId) != 1)
			return 1;
		msg::ObjectPropsSupported ops = session->GetObjectPropsSupported(objectId);
		printf("properties supported: ");
		for(u16 prop: ops.ObjectPropCodes)
		{
			printf("%02x ", prop);
		}
		printf("\n");
	}
	else if (command == "device-properties")
	{
		for(u16 code : gdi.DevicePropertiesSupported)
		{
			if ((code & 0xff00) != 0x5000 )
				continue;
			printf("property code: %04x\n", (unsigned)code);
			ByteArray data = session->GetDeviceProperty((mtp::DeviceProperty)code);
			HexDump("value", data);
		}
	}
#endif

}

#include <stdio.h>
#include <stdexcept>

#include <mtp/usb/BulkPipe.h>
#include <mtp/usb/Context.h>
#include <mtp/usb/call.h>
#include <mtp/ptp/Container.h>
#include <mtp/ptp/Device.h>
#include <mtp/ptp/Messages.h>

int main(int argc, char **argv)
{
	using namespace mtp;
	//USB_CALL(libusb_reset_device(device->GetHandle()));

	//printf("claiming interface %d...\n", interface->GetIndex());
	//USB_CALL(libusb_claim_interface(device->GetHandle(), interface->GetIndex()));
	//printf("claimed interface\n");
	//USB_CALL(libusb_set_interface_alt_setting(device->GetHandle(), mtp_interface, 0));

	DevicePtr mtp(Device::Find());
	if (!mtp)
	{
		printf("no mtp device found\n");
		return 1;
	}

	msg::DeviceInfo gdi = mtp->GetDeviceInfo();
	printf("%s\n", gdi.VendorExtensionDesc.c_str());
	printf("%s ", gdi.Manufactorer.c_str());
	printf("%s ", gdi.Model.c_str());
	printf("%s", gdi.DeviceVersion.c_str());
	printf("%s\n", gdi.SerialNumber.c_str());
	printf("supported op codes: ");
	for(u16 code : gdi.OperationsSupported)
	{
		printf("%04x ", (unsigned)code);
	}
	printf("\n");

	if (argc < 2)
		return 0;

	SessionPtr session = mtp->OpenSession(1);
	std::string command = argv[1];
	if (command == "list")
	{
		mtp::u32 parent = mtp::Session::Root;
		if (argc > 2)
			if (sscanf(argv[2], "%x", &parent) != 1)
				return 1;

		msg::ObjectHandles handles = session->GetObjectHandles(mtp::Session::AllStorages, mtp::Session::AllFormats, parent);

		for(u32 objectId : handles.ObjectHandles)
		{
			try
			{
				msg::ObjectInfo info = session->GetObjectInfo(objectId);
				printf("%08x %04x %s %ux%u, %s\n", objectId, info.ObjectFormat, info.Filename.c_str(), info.ImagePixWidth, info.ImagePixHeight, info.CaptureDate.c_str());
			}
			catch(const std::exception &ex)
			{
				printf("error: %s\n", ex.what());
			}
		}
		//libusb_release_interface(device->GetHandle(), interface->GetIndex());
	}
	else if (command == "get")
	{
		if (argc < 3)
			return 1;
		mtp::u32 objectId;
		if (sscanf(argv[2], "%x", &objectId) != 1)
			return 1;
		printf("object id = %08x\n", objectId);
		msg::ObjectInfo info = session->GetObjectInfo(objectId);
		printf("filename = %s\n", info.Filename.c_str());
		FILE *f = fopen(info.Filename.c_str(), "wb");
		if (!f)
		{
			perror("open");
			return 1;
		}
		ByteArray object = session->GetObject(objectId);
		if (fwrite(object.data(), object.size(), 1, f) != 1)
			perror("fwriter");
		fclose(f);
	}
	else if (command == "set")
	{
		if (argc < 4)
			return 1;

		mtp::u32 objectId;
		if (sscanf(argv[2], "%x", &objectId) != 1)
			return 1;

		std::string filename(argv[3]);
		printf("uploading %s to %08x\n", filename.c_str(), objectId);
		FILE *f = fopen(filename.c_str(), "rb");
		if (!f)
		{
			perror("open");
			return 1;
		}
		msg::ObjectInfo oi;
		oi.Filename = filename;
		fclose(f);
	}

	return 0;
}

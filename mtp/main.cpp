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
	printf("%s\n", gdi.Manufactorer.c_str());
	printf("%s\n", gdi.Model.c_str());
	printf("%s\n", gdi.DeviceVersion.c_str());
	printf("%s\n", gdi.SerialNumber.c_str());
	for(u16 code : gdi.OperationsSupported)
	{
		printf("supported op code: %04x\n", (unsigned)code);
	}

	SessionPtr session = mtp->OpenSession(1);
	msg::ObjectHandles handles = session->GetObjectHandles();

	for(u32 objectId : handles.ObjectHandles)
	{
		try
		{
			printf("GET OBJECT ID INFO 0x%08x\n", objectId);
			msg::ObjectInfo info = session->GetObjectInfo(objectId);
			printf("%04x %s %ux%u, parent: 0x%08x\n", info.ObjectFormat, info.Filename.c_str(), info.ImagePixWidth, info.ImagePixHeight, info.ParentObject);
		}
		catch(const std::exception &ex)
		{
			printf("error: %s\n", ex.what());
		}
	}
	//libusb_release_interface(device->GetHandle(), interface->GetIndex());

	return 0;
}

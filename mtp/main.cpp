#include <stdio.h>
#include <sstream>
#include <stdexcept>

#include <mtp/usb/BulkPipe.h>
#include <mtp/usb/Context.h>
#include <mtp/usb/call.h>
#include <mtp/ptp/OperationRequest.h>
#include <mtp/ptp/Container.h>
#include <mtp/ptp/Messages.h>
#include <mtp/ptp/Protocol.h>

int main(int argc, char **argv)
{
	using namespace mtp;
	usb::Context ctx;
	usb::DeviceDescriptorPtr desc;
	for (usb::DeviceDescriptorPtr dd : ctx.GetDevices())
	{
		if (dd->GetVendorId() == 0x18d1)
		{
			desc = dd;
			break;
		}
	}
	if (!desc)
		throw std::runtime_error("no mtp device found");

	usb::DevicePtr device = desc->Open();
	int confs = desc->GetConfigurationsCount();
	printf("configurations: %d\n", confs);

	int mtp_configuration = -1;
	int mtp_interface = -1;

	usb::ConfigurationPtr		configuration;
	usb::InterfacePtr			interface;

	for(int i = 0; i < confs; ++i)
	{
		usb::ConfigurationPtr conf = desc->GetConfiguration(i);
		int interfaces = conf->GetInterfaceCount();
		printf("interfaces: %d\n", interfaces);
		for(int j = 0; j < interfaces; ++j)
		{
			usb::InterfacePtr iface = conf->GetInterface(conf, j, 0);
			printf("%d:%d index %u, eps %u\n", i, j, iface->GetIndex(), iface->GetEndpointsCount());
			int name_idx = iface->GetNameIndex();
			if (!name_idx)
				continue;
			std::string name = device->GetString(name_idx);
			if (name == "MTP")
			{
				configuration = conf;
				interface = iface;
				mtp_configuration = i;
				mtp_interface = j;
				i = confs;
				break;
			}
		}
	}

	if (!interface || mtp_interface < 0 || mtp_configuration < 0)
		throw std::runtime_error("no mtp interface found");

	//device->SetConfiguration(configuration->GetIndex());
	usb::BulkPipePtr pipe = usb::BulkPipe::Create(device, interface);
	//USB_CALL(libusb_reset_device(device->GetHandle()));

	printf("claiming interface %d...\n", interface->GetIndex());
	//USB_CALL(libusb_claim_interface(device->GetHandle(), interface->GetIndex()));
	printf("claimed interface\n");
	//USB_CALL(libusb_set_interface_alt_setting(device->GetHandle(), mtp_interface, 0));

	Protocol proto(pipe);

	{
		OperationRequest req(OperationCode::GetDeviceInfo, 0, 0);
		Container container(req);
		proto.Write(container.Data);
		ByteArray data = proto.Read();
		HexDump("payload", data);

		Stream stream(data, 6); //operation code + session id
		GetDeviceInfo gdi;
		gdi.Read(stream);
		printf("%s\n", gdi.VendorExtensionDesc.c_str());
		printf("%s\n", gdi.Manufactorer.c_str());
		printf("%s\n", gdi.Model.c_str());
		printf("%s\n", gdi.DeviceVersion.c_str());
		printf("%s\n", gdi.SerialNumber.c_str());
		for(u16 code : gdi.OperationsSupported)
		{
			printf("supported op code: %04x\n", (unsigned)code);
		}
	}

	{
		OperationRequest req(OperationCode::OpenSession, 0, 1);
		Container container(req);
		proto.Write(container.Data);
		ByteArray data = proto.Read();
		HexDump("payload", data);
	}

	{
		OperationRequest req(OperationCode::GetObjectHandles, 1, 0xffffffffu);
		Container container(req);
		proto.Write(container.Data);
		ByteArray data = proto.Read();
		HexDump("payload", data);
	}

	//libusb_release_interface(device->GetHandle(), interface->GetIndex());

	return 0;
}

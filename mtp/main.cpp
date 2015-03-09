#include <stdio.h>
#include <sstream>
#include <stdexcept>

#include <mtp/usb/Context.h>
#include <mtp/usb/call.h>
#include <mtp/ptp/OperationRequest.h>

static void callback(struct libusb_transfer *transfer)
{
	printf("CALLBACK!!!\n");
}

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
			usb::InterfacePtr iface = conf->GetInterface(j, 0);
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

	device->SetConfiguration(configuration->GetIndex());
	int epn = interface->GetEndpointsCount();

	usb::EndpointPtr out;
	printf("endpoints: %d\n", epn);
	for(int i = 0; i < epn; ++i)
	{
		usb::EndpointPtr ep = interface->GetEndpoint(i);
		printf("endpoint: %d: %02x\n", i, ep->GetAddress());
		//check for bulk here
		if (ep->GetDirection() == usb::EndpointDirection::Out)
		{
			printf("OUT\n");
			out = ep;
			break;
		}
	}

	printf("claiming interface %d...\n", interface->GetIndex());
	USB_CALL(libusb_claim_interface(device->GetHandle(), interface->GetIndex()));
	printf("claimed interface\n");
	//USB_CALL(libusb_set_interface_alt_setting(device->GetHandle(), mtp_interface, 0));

	OperationRequest req(OperationCode::GetDeviceInfo);
	libusb_transfer *transfer = libusb_alloc_transfer(0);
	printf("data size: %u\n", (unsigned)req.Data.size());
	libusb_fill_bulk_transfer(transfer, device->GetHandle(), out->GetAddress(),
			req.Data.data(), req.Data.size(), &callback, 0, 3000);
	USB_CALL(libusb_submit_transfer(transfer));
	printf("submitted\n");

	while(true);
	return 0;
}

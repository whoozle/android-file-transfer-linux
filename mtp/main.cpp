#include <stdio.h>
#include <sstream>
#include <stdexcept>

#include <mtp/usb/Context.h>
#include <mtp/usb/call.h>
#include <mtp/ptp/OperationRequest.h>
#include <mtp/ptp/Container.h>

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

	usb::EndpointPtr out, in, interrupt;
	printf("endpoints: %d\n", epn);
	for(int i = 0; i < epn; ++i)
	{
		usb::EndpointPtr ep = interface->GetEndpoint(i);
		printf("endpoint: %d: %02x\n", i, ep->GetAddress());
		//check for bulk here
		if (ep->GetDirection() == usb::EndpointDirection::Out)
		{
			if (ep->GetType() == usb::EndpointType::Bulk)
			{
				printf("OUT\n");
				out = ep;
			}
		}
		else
		{
			if (ep->GetType() == usb::EndpointType::Bulk)
			{
				printf("IN\n");
				in = ep;
			}
			else
			{
				printf("INTERRUPT\n");
				interrupt = ep;
			}
		}
	}
	if (!in || !out || !interrupt)
		throw std::runtime_error("invalid endpoint");

	printf("claiming interface %d...\n", interface->GetIndex());
	USB_CALL(libusb_claim_interface(device->GetHandle(), interface->GetIndex()));
	printf("claimed interface\n");
	//USB_CALL(libusb_set_interface_alt_setting(device->GetHandle(), mtp_interface, 0));

	OperationRequest req(OperationCode::GetDeviceInfo);
	Container container(req);

	std::vector<u8> data;
	data.resize(4096);
	libusb_transfer *transfer_in = libusb_alloc_transfer(0);
	libusb_fill_bulk_transfer(transfer_in, device->GetHandle(), in->GetAddress(), data.data(), data.size(), &callback, 0, 3000);
	USB_CALL(libusb_submit_transfer(transfer_in));

	libusb_transfer *transfer_out = libusb_alloc_transfer(0);
	printf("data size: %u\n", (unsigned)container.Data.size());
	libusb_fill_bulk_transfer(transfer_out, device->GetHandle(), out->GetAddress(), container.Data.data(), container.Data.size(), &callback, 0, 3000);
	USB_CALL(libusb_submit_transfer(transfer_out));

	while(true);
	return 0;
}

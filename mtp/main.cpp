#include <stdio.h>
#include <sstream>
#include <stdexcept>

#include <mtp/usb/Context.h>

int main(int argc, char **argv)
{
	mtp::usb::Context ctx;
	mtp::usb::DeviceDescriptorPtr desc;
	for (mtp::usb::DeviceDescriptorPtr dd : ctx.GetDevices())
	{
		if (dd->GetVendorId() == 0x18d1)
		{
			desc = dd;
			break;
		}
	}
	if (!desc)
		throw std::runtime_error("no mtp device found");
	int confs = desc->GetConfigurationsCount();
	printf("configurations: %d\n", confs);
	for(int i = 0; i < confs; ++i)
	{
		mtp::usb::ConfigurationPtr conf = desc->GetConfiguration(i);
		int interfaces = conf->GetInterfaceCount();
		printf("interfaces: %d\n", interfaces);
		for(int j = 0; j < interfaces; ++j)
		{
			mtp::usb::InterfacePtr interface = conf->GetInterface(j, 0);
			printf("endpoints: %d\n", interface->GetEndpointsCount());
		}
	}
	return 0;
}

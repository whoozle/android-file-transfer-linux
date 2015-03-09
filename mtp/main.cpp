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

	mtp::usb::DevicePtr device = desc->Open();
	int confs = desc->GetConfigurationsCount();
	printf("configurations: %d\n", confs);

	int mtp_configuration = -1;
	int mtp_interface = -1;
	for(int i = 0; i < confs; ++i)
	{
		mtp::usb::ConfigurationPtr conf = desc->GetConfiguration(i);
		int interfaces = conf->GetInterfaceCount();
		printf("interfaces: %d\n", interfaces);
		for(int j = 0; j < interfaces; ++j)
		{
			mtp::usb::InterfacePtr interface = conf->GetInterface(j, 0);
			int name_idx = interface->GetNameIndex();
			if (!name_idx)
				continue;
			std::string name = device->GetString(name_idx);
			if (name == "MTP")
			{
				mtp_configuration = i;
				mtp_interface = j;
				break;
			}
		}
	}

	if (mtp_interface < 0 || mtp_configuration < 0)
		throw std::runtime_error("no mtp interface found");

	device->SetConfiguration(mtp_configuration);

	return 0;
}

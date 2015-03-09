#include <stdio.h>
#include <sstream>
#include <stdexcept>

#include <mtp/usb/Context.h>

int main() {
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
	return 0;
}

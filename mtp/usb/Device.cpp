#include <mtp/usb/Device.h>
#include <mtp/usb/call.h>

namespace mtp
{

	Device::Device(libusb_device_handle * handle): _handle(handle)
	{}

	Device::~Device()
	{
		libusb_close(_handle);
	}

}
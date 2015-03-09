#include <mtp/usb/Context.h>
#include <mtp/usb/call.h>
#include <stdio.h>

namespace mtp { namespace usb
{

	Context::Context()
	{
		USB_CALL(libusb_init(&_ctx));
		libusb_set_debug(_ctx, 3);
		libusb_device **devs;
		int count = libusb_get_device_list(_ctx, &devs);
		if (count < 0)
			throw Exception("libusb_get_device_list", count);

		_devices.reserve(count);
		for(int i = 0; i < count; ++i)
			_devices.push_back(std::make_shared<DeviceDescriptor>(devs[i]));
		libusb_free_device_list(devs, 0);
	}

	Context::~Context()
	{
		libusb_exit(_ctx);
	}

}}

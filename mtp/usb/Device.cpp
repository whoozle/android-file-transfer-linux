#include <mtp/usb/Device.h>
#include <mtp/usb/call.h>

namespace mtp { namespace usb
{

	Device::Device(libusb_device_handle * handle): _handle(handle)
	{}

	Device::~Device()
	{
		libusb_close(_handle);
	}

	void Device::SetConfiguration(int idx)
	{
		USB_CALL(libusb_set_configuration(_handle, idx));
	}

	std::string Device::GetString(int idx) const
	{
		unsigned char buffer[4096];
		int r = libusb_get_string_descriptor_ascii(_handle, idx, buffer, sizeof(buffer));
		if (r < 0)
			throw Exception(r);
		return std::string(buffer, buffer + r);
	}

}}
#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <libusb.h>
#include <mtp/types.h>
#include <mtp/usb/Device.h>

namespace mtp { namespace usb
{

	class DeviceDescriptor
	{
	private:
		libusb_device			*	_dev;
		libusb_device_descriptor	_descriptor;

	public:
		DeviceDescriptor(libusb_device *dev);
		~DeviceDescriptor();

		DevicePtr Open();
	};
	DECLARE_PTR(DeviceDescriptor);

}}

#endif


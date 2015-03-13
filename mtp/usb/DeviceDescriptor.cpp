#include <mtp/usb/DeviceDescriptor.h>
#include <mtp/usb/call.h>

namespace mtp { namespace usb
{
	DeviceDescriptor::DeviceDescriptor(libusb_device *dev): _dev(dev)
	{
		USB_CALL(libusb_get_device_descriptor(_dev, &_descriptor));
	}

	ConfigurationPtr DeviceDescriptor::GetConfiguration(int conf)
	{
		libusb_config_descriptor *desc;
		USB_CALL(libusb_get_config_descriptor(_dev, conf, &desc));
		return std::make_shared<Configuration>(desc);
	}

	DevicePtr DeviceDescriptor::Open()
	{
		libusb_device_handle *handle;
		USB_CALL(libusb_open(_dev, &handle));
		return std::make_shared<Device>(handle);
	}

	DevicePtr DeviceDescriptor::TryOpen()
	{
		libusb_device_handle *handle;
		int r = libusb_open(_dev, &handle);
		return r == 0? std::make_shared<Device>(handle): nullptr;
	}

	DeviceDescriptor::~DeviceDescriptor()
	{ libusb_unref_device(_dev); }

}}

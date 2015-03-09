#include <mtp/usb/DeviceDescriptor.h>
#include <mtp/usb/call.h>

namespace mtp { namespace usb
{
	DeviceDescriptor::DeviceDescriptor(libusb_device *dev): _dev(dev)
	{
		USB_CALL(libusb_get_device_descriptor(_dev, &_descriptor));
		printf("%04x:%04x\n", _descriptor.idVendor, _descriptor.idProduct);
		for(unsigned config = 0; config < _descriptor.bNumConfigurations; ++config)
		{
			libusb_config_descriptor *desc;
			USB_CALL(libusb_get_config_descriptor(_dev, config, &desc));
			for(unsigned i = 0; i < desc->bNumInterfaces; ++i)
			{
				const libusb_interface &interface = desc->interface[i];
				for(int j = 0; j < interface.num_altsetting; ++j)
				{
					const libusb_interface_descriptor & interface_desc = interface.altsetting[j];
					printf("\t%d: %u %u\n", j, interface_desc.bInterfaceClass, interface_desc.iInterface);
				}
			}
		}
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

	DeviceDescriptor::~DeviceDescriptor()
	{ libusb_unref_device(_dev); }

}}

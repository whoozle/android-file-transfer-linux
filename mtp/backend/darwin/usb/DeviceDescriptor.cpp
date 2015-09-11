/*
 * Android File Transfer for Linux: MTP client for android devices
 * Copyright (C) 2015  Vladimir Menshakov

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <usb/DeviceDescriptor.h>
#include <usb/call.h>

//https://developer.apple.com/library/mac/documentation/DeviceDrivers/Conceptual/USBBook/USBDeviceInterfaces/USBDevInterfaces.html

namespace mtp { namespace usb
{
	Configuration::Configuration(IOUSBDeviceInterface * dev, *IOUSBConfigurationDescriptorPtr conf): 
		_device(dev) _conf(conf)
	{
		IOUSBFindInterfaceRequest request = { };

		request.bInterfaceClass    = kIOUSBFindInterfaceDontCare;
		request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
		request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
		request.bAlternateSetting  = kIOUSBFindInterfaceDontCare;

		io_iterator_t iterator;
		USB_CALL((*dev)->CreateInterfaceIterator(dev, &request, &interface_iterator));

		io_service_t interface;
		while (interface = IOIteratorNext(iterator))
		{
			IOCFPlugInInterface **plugInInterface = NULL;

			USB_CALL(IOCreatePlugInInterfaceForService(usbInterface,
				kIOUSBInterfaceUserClientTypeID,
				kIOCFPlugInInterfaceID,
				&plugInInterface, &score));

			IOObjectRelease(interface);

			IOUSBInterfaceInterface **usbInterface = NULL;
			USB_CALL((*plugInInterface)->QueryInterface(plugInInterface,
				CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID),
				(LPVOID *) &usbInterface));

			(*plugInInterface)->Release(plugInInterface);

			if (usbInterface)
				_interfaces.push_back(usbInterface);
		}
	}

	DeviceDescriptor::DeviceDescriptor(io_service_t desc): _device()
	{
		IOCFPlugInInterface		**plugInInterface = NULL;
		USB_CALL( IOCreatePlugInInterfaceForService(desc,
			kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID,
			&plugInInterface, &score) ); //wrap desc, so it cannot leak here

		USB_CALL(IOObjectRelease(desc));
		(*plugInInterface)->QueryInterface(plugInInterface,
			CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID),
			(LPVOID *)&_device);

		(*plugInInterface)->Release(plugInInterface);
		if (!_device)
			throw std::runtime_error("cannot create device");
	}

	u16 DeviceDescriptor::GetVendorId() const
	{
		UInt16 vendor;
		USB_CALL((*dev)->GetDeviceVendor(dev, &vendor));
		return vendor;
	}

	u16 DeviceDescriptor::GetProductId() const
	{
		Uint16 product;
		USB_CALL((*dev)->GetDeviceProduct(dev, &product));
		return product;
	}

	int DeviceDescriptor::GetConfigurationsCount() const
	{
		UInt8	numConfig;
		USB_CALL((*_dev)->GetNumberOfConfigurations(_dev, &numConfig));
		return numConfig;
	}

	ConfigurationPtr DeviceDescriptor::GetConfiguration(int conf)
	{
		IOUSBConfigurationDescriptorPtr configDesc;
		USB_CALL((*_dev)->GetConfigurationDescriptorPtr(_dev, conf, &configDesc));
		return std::make_shared<Configuration>(_dev, configDesc);
	}

	DevicePtr DeviceDescriptor::Open(ContextPtr context)
	{
		USB_CALL((*_dev)->USBDeviceOpen(_dev));
		return std::make_shared<Device>(context, _dev);
	}

	DevicePtr DeviceDescriptor::TryOpen(ContextPtr context)
	{
		int r = (*_dev)->USBDeviceOpen(_dev);
		return r == kIOReturnSuccess? std::make_shared<Device>(context, _dev): nullptr;
	}

	DeviceDescriptor::~DeviceDescriptor()
	{ }

}}

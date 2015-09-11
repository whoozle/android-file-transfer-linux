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
	Configuration::Configuration(IOUSBDeviceInterface ** dev, IOUSBConfigurationDescriptorPtr conf):
		_dev(dev), _conf(conf)
	{
		IOUSBFindInterfaceRequest request = { };

		request.bInterfaceClass    = kIOUSBFindInterfaceDontCare;
		request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
		request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
		request.bAlternateSetting  = kIOUSBFindInterfaceDontCare;

		io_iterator_t iterator;
		USB_CALL((*dev)->CreateInterfaceIterator(dev, &request, &iterator));

		io_service_t interface;
		while ((interface = IOIteratorNext(iterator)))
		{
			IOCFPlugInInterface **plugInInterface = NULL;
			SInt32 score;
			USB_CALL(IOCreatePlugInInterfaceForService(interface,
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

	DeviceDescriptor::DeviceDescriptor(io_service_t desc): _dev()
	{
		IOCFPlugInInterface **plugInInterface = NULL;
		SInt32 score;
		USB_CALL( IOCreatePlugInInterfaceForService(desc,
			kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID,
			&plugInInterface, &score) ); //wrap desc, so it cannot leak here

		USB_CALL(IOObjectRelease(desc));
		(*plugInInterface)->QueryInterface(plugInInterface,
			CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID),
			(LPVOID *)&_dev);

		(*plugInInterface)->Release(plugInInterface);
		if (!_dev)
			throw std::runtime_error("cannot create device");
	}

	u16 DeviceDescriptor::GetVendorId() const
	{
		UInt16 vendor;
		USB_CALL((*_dev)->GetDeviceVendor(_dev, &vendor));
		return vendor;
	}

	u16 DeviceDescriptor::GetProductId() const
	{
		UInt16 product;
		USB_CALL((*_dev)->GetDeviceProduct(_dev, &product));
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

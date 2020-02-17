/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2020  Vladimir Menshakov

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <usb/DeviceDescriptor.h>
#include <usb/call.h>
#include <mtp/log.h>

//https://developer.apple.com/library/mac/documentation/DeviceDrivers/Conceptual/USBBook/USBDeviceInterfaces/USBDevInterfaces.html

namespace mtp { namespace usb
{
	Configuration::Configuration(IOUSBDeviceType ** dev, IOUSBConfigurationDescriptorPtr conf):
		_conf(conf)
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
			CFUUIDGetUUIDBytes(DeviceInterfaceID),
			(LPVOID *)&_dev);

		(*plugInInterface)->Release(plugInInterface);
		if (!_dev)
			throw std::runtime_error("cannot create device");

		bool wakeup = true;
		if (GetVendorId() == kIOUSBVendorIDAppleComputer)
			wakeup = false;
#if defined (kIOUSBInterfaceInterfaceID500)
		if (wakeup)
		{
			UInt32 info = 0;
			int r = (*_dev)->GetUSBDeviceInformation (_dev, &info);
			if (r == kIOReturnSuccess && (info & (1 << kUSBInformationDeviceIsSuspendedBit)) == 0)
				wakeup = false;
		}
#endif
		if (wakeup)
		{
			(*_dev)->USBDeviceSuspend (_dev, 0);
		}
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
		if (r == kIOReturnSuccess)
			return std::make_shared<Device>(context, _dev);

		debug("USBDeviceOpen failed: ", Exception::GetErrorMessage(r));
		return nullptr;
	}

	DeviceDescriptor::~DeviceDescriptor()
	{ }

	ByteArray DeviceDescriptor::GetDescriptor()
	{
		ByteArray data(255);
		Device::ReadControl(_dev, 0x80, 6, 0x200, 0, data, 1000);
		return data;
	}

}}

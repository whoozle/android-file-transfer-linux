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

#ifndef AFTL_MTP_BACKEND_DARWIN_USB_DEVICEDESCRIPTOR_H
#define AFTL_MTP_BACKEND_DARWIN_USB_DEVICEDESCRIPTOR_H

#include <mtp/types.h>
#include <usb/Device.h>
#include <usb/Interface.h>

#include <usb/usb.h>

#include <vector>

namespace mtp { namespace usb
{
	class Configuration : Noncopyable
	{
		//IOUSBDeviceType			**_dev;
		IOUSBConfigurationDescriptorPtr	_conf;

		std::vector<IOUSBInterfaceInterface **> _interfaces;

	public:
		Configuration(IOUSBDeviceType ** dev, IOUSBConfigurationDescriptorPtr conf);

		int GetIndex() const
		{ return _conf->bConfigurationValue; }

		int GetInterfaceCount() const
		{ return _interfaces.size(); }

		int GetInterfaceAltSettingsCount(int idx) const
		{ return 1; }

		InterfacePtr GetInterface(DevicePtr device, ConfigurationPtr config, int idx, int settings) const
		{ return std::make_shared<Interface>(device, config, _interfaces.at(idx)); }
	};

	class DeviceDescriptor
	{
	private:
		IOUSBDeviceType			**_dev;

	public:
		DeviceDescriptor(io_service_t desc);
		~DeviceDescriptor();

		u16 GetVendorId() const;
		u16 GetProductId() const;

		DevicePtr Open(ContextPtr context);
		DevicePtr TryOpen(ContextPtr context);

		int GetConfigurationsCount() const;

		ConfigurationPtr GetConfiguration(int conf);
		ByteArray GetDescriptor();
	};
	DECLARE_PTR(DeviceDescriptor);

}}

#endif


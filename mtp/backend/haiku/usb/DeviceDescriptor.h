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

#ifndef AFTL_MTP_BACKEND_LIBUSB_USB_DEVICEDESCRIPTOR_H
#define AFTL_MTP_BACKEND_LIBUSB_USB_DEVICEDESCRIPTOR_H

#include <mtp/types.h>
#include <usb/Device.h>
#include <usb/Interface.h>

namespace mtp { namespace usb
{
	class Configuration : Noncopyable
	{
		const BUSBConfiguration *_config;

	public:
		Configuration(const BUSBConfiguration *config) : _config(config) { }
		~Configuration() { /* configuration is owned by its parent device */ }

		int GetIndex() const
		{ return _config->Index(); }

		int GetInterfaceCount() const
		{ return _config->CountInterfaces(); }

		int GetInterfaceAltSettingsCount(int idx) const
		{ return _config->InterfaceAt(idx)->CountAlternates(); }

		// TODO figure out what to do with "int settings"
		InterfacePtr GetInterface(DevicePtr device, ConfigurationPtr config, int idx, int settings) const
		{ return std::make_shared<Interface>(device, config, *_config->InterfaceAt(idx)->AlternateAt(settings)); }
	};

	class DeviceDescriptor
	{
	private:
		BUSBDevice			*	_dev;

	public:
		DeviceDescriptor(BUSBDevice *dev);
		~DeviceDescriptor();

		u16 GetVendorId() const
		{ return _dev->VendorID(); }

		u16 GetProductId() const
		{ return _dev->ProductID(); }

		DevicePtr Open(ContextPtr context);
		DevicePtr TryOpen(ContextPtr context);

		int GetConfigurationsCount() const
		{ return _dev->CountConfigurations(); }

		ConfigurationPtr GetConfiguration(int conf);

		ByteArray GetDescriptor() const;
	};
	DECLARE_PTR(DeviceDescriptor);

}}

#endif


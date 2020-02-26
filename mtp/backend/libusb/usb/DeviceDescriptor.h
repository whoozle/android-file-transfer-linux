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

#include <libusb.h>
#include <mtp/types.h>
#include <usb/Device.h>
#include <usb/Interface.h>

namespace mtp { namespace usb
{
	class Configuration : Noncopyable
	{
		libusb_config_descriptor *_config;

	public:
		Configuration(libusb_config_descriptor *config) : _config(config) { }
		~Configuration() { libusb_free_config_descriptor(_config); }

		int GetIndex() const
		{ return _config->bConfigurationValue; }

		int GetInterfaceCount() const
		{ return _config->bNumInterfaces; }

		int GetInterfaceAltSettingsCount(int idx) const
		{ return _config->interface[idx].num_altsetting; }

		InterfacePtr GetInterface(DevicePtr device, ConfigurationPtr config, int idx, int settings) const
		{ return std::make_shared<Interface>(device, config, _config->interface[idx].altsetting[settings]); }
	};

	class DeviceDescriptor
	{
	private:
		libusb_device			*	_dev;
		libusb_device_descriptor	_descriptor;

	public:
		DeviceDescriptor(libusb_device *dev);
		~DeviceDescriptor();

		u16 GetVendorId() const
		{ return _descriptor.idVendor; }

		u16 GetProductId() const
		{ return _descriptor.idProduct; }

		DevicePtr Open(ContextPtr context);
		DevicePtr TryOpen(ContextPtr context);

		int GetConfigurationsCount() const
		{ return _descriptor.bNumConfigurations; }

		ConfigurationPtr GetConfiguration(int conf);

		ByteArray GetDescriptor() const;
	};
	DECLARE_PTR(DeviceDescriptor);

}}

#endif


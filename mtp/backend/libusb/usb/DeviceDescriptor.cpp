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

	DevicePtr DeviceDescriptor::Open(ContextPtr context)
	{
		libusb_device_handle *handle;
		USB_CALL(libusb_open(_dev, &handle));
		return std::make_shared<Device>(context, handle);
	}

	DevicePtr DeviceDescriptor::TryOpen(ContextPtr context)
	{
		libusb_device_handle *handle;
		int r = libusb_open(_dev, &handle);
		return r == 0? std::make_shared<Device>(context, handle): nullptr;
	}

	DeviceDescriptor::~DeviceDescriptor()
	{ libusb_unref_device(_dev); }

	ByteArray DeviceDescriptor::GetDescriptor() const
	{
#warning not implemented
		ByteArray stub; return stub;
	}

}}

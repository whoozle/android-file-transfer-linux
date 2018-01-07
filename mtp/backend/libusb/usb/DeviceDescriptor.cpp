/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2018  Vladimir Menshakov

    Android File Transfer For Linux is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    Android File Transfer For Linux is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Android File Transfer For Linux.
    If not, see <http://www.gnu.org/licenses/>.
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
	{ throw std::runtime_error("not possible with libusb"); }

}}

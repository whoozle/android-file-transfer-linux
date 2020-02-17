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

#include <usb/Context.h>
#include <usb/call.h>
#include <stdio.h>

namespace mtp { namespace usb
{

	Context::Context(int debugLevel)
	{
		USB_CALL(libusb_init(&_ctx));
		libusb_set_debug(_ctx, debugLevel);
		libusb_device **devs;
		int count = libusb_get_device_list(_ctx, &devs);
		if (count < 0)
			throw Exception("libusb_get_device_list", count);

		_devices.reserve(count);
		for(int i = 0; i < count; ++i)
			_devices.push_back(std::make_shared<DeviceDescriptor>(devs[i]));
		libusb_free_device_list(devs, 0);
	}

	Context::~Context()
	{
		libusb_exit(_ctx);
	}

	void Context::Wait()
	{
		USB_CALL(libusb_handle_events(_ctx));
	}


}}

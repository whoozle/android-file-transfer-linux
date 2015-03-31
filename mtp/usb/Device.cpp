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
#include <mtp/usb/Device.h>
#include <mtp/usb/Context.h>
#include <mtp/usb/call.h>

namespace mtp { namespace usb
{

	Device::Device(ContextPtr context, libusb_device_handle * handle): _context(context), _handle(handle)
	{}

	Device::~Device()
	{
		libusb_close(_handle);
	}

	int Device::GetConfiguration() const
	{
		int config;
		USB_CALL(libusb_get_configuration(_handle, &config));
		return config;
	}

	void Device::SetConfiguration(int idx)
	{
		USB_CALL(libusb_set_configuration(_handle, idx));
	}

	std::string Device::GetString(int idx) const
	{
		unsigned char buffer[4096];
		int r = libusb_get_string_descriptor_ascii(_handle, idx, buffer, sizeof(buffer));
		if (r < 0)
			throw Exception("libusb_get_string_descriptor_ascii", r);
		return std::string(buffer, buffer + r);
	}

}}
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
#include <usb/Context.h>
#include <stdio.h>
#include <usb/Directory.h>

namespace mtp { namespace usb
{

	Context::Context()
	{
		std::string bus("/sys/bus/usb/devices");
		Directory usbDir(bus);
		while(true)
		{
			std::string entry = usbDir.Read();
			if (entry.empty())
				break;
			if (entry.compare(0, 3, "usb") != 0)
				continue;

			_devices.push_back(std::make_shared<DeviceDescriptor>(bus + "/" + entry));
		}
	}

	Context::~Context()
	{
	}


}}

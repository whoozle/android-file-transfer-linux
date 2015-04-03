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
#include <usb/Directory.h>
#include <map>
#include <stdio.h>

namespace mtp { namespace usb
{

	Context::Context()
	{
		std::string rootPath("/sys/bus/usb/devices");
		Directory usbDir(rootPath);
		std::map<int, std::map<int, DeviceDescriptorPtr> > devices;
		while(true)
		{
			std::string entry = usbDir.Read();
			if (entry.empty())
				break;

			unsigned busId, port, conf, interface;
			if (sscanf(entry.c_str(), "%u-%u:%u.%u", &busId, &port, &conf, &interface) == 4 && port != 0)
			{
				DeviceDescriptorPtr &device = devices[busId][port];
				if (!device)
				{
					char deviceRoot[64];
					snprintf(deviceRoot, sizeof(deviceRoot), "%u-%u", busId, port);
					device = std::make_shared<DeviceDescriptor>(rootPath + "/" + std::string(deviceRoot));
					_devices.push_back(device);
				}
			}

		}
	}

	Context::~Context()
	{
	}


}}

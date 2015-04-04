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
#include <usb/Directory.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace mtp { namespace usb
{
	DeviceDescriptor::DeviceDescriptor(int busId, const std::string &path): _busId(busId), _path(path)
	{
		_vendor			= Directory::ReadInt(path + "/idVendor");
		_product		= Directory::ReadInt(path + "/idProduct");
		_deviceNumber	= Directory::ReadInt(path + "/devnum", 10);
		printf("device at %s\n", path.c_str());
	}

	DevicePtr DeviceDescriptor::Open(ContextPtr context)
	{
		DevicePtr device = TryOpen(context);
		if (!device)
			throw std::runtime_error("cannot open device at " + _path);
		return device;
	}

	DevicePtr DeviceDescriptor::TryOpen(ContextPtr context)
	{
		char fname[256];
		snprintf(fname, sizeof(fname), "/dev/bus/usb/%03d/%03u", _busId, _deviceNumber);
		int fd = open(fname, O_RDWR);
		return fd != -1? std::make_shared<Device>(fd): nullptr;
	}

	DeviceDescriptor::~DeviceDescriptor()
	{ }

}}

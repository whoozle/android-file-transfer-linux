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
#include <usb/Device.h>
#include <usb/Exception.h>

#include <unistd.h>
#include <sys/ioctl.h>

#include "linux/usbdevice_fs.h"

namespace mtp { namespace usb
{
	Device::InterfaceToken::InterfaceToken(int fd, unsigned interfaceNumber): _fd(fd), _interfaceNumber(interfaceNumber)
	{
		int r = ioctl(_fd, USBDEVFS_CLAIMINTERFACE, &interfaceNumber);
		if (r < 0)
			throw Exception("ioctl(USBDEVFS_CLAIMINTERFACE)");
	}

	Device::InterfaceToken::~InterfaceToken()
	{
		ioctl(_fd, USBDEVFS_RELEASEINTERFACE, _interfaceNumber);
	}

	Device::Device(int fd): _fd(fd)
	{

	}

	Device::~Device()
	{
		close(_fd);
	}

	int Device::GetConfiguration() const
	{
		return 0;
	}

	void Device::SetConfiguration(int idx)
	{
		fprintf(stderr, "SetConfiguration(%d): not implemented", idx);
	}

}}
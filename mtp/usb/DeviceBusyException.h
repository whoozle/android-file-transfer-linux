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

#ifndef AFTL_MTP_USB_DEVICEBUSYEXCEPTION_H
#define AFTL_MTP_USB_DEVICEBUSYEXCEPTION_H

#include <stdexcept>
#include <vector>
#include <string>

namespace mtp { namespace usb
{

	struct DeviceBusyException : public std::runtime_error //! Exception thrown when device is busy (claimed by other process)
	{
		struct ProcessDescriptor
		{
			int Id;
			std::string Name;
		};

		std::vector<ProcessDescriptor> Processes;

		DeviceBusyException(int fd = -1, const std::string &msg = "Device is already used by another process");

		void Kill() const;
		static void Kill(const ProcessDescriptor & desc);
		static void Kill(const std::vector<ProcessDescriptor> & processes);
	};

}}

#endif

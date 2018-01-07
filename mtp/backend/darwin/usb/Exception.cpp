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

#include <usb/Exception.h>
#include <usb/usb.h>
#include <string>

namespace mtp { namespace usb
{
	Exception::Exception(const std::string &what, int returnCode) throw() : std::runtime_error(what + ": " + GetErrorMessage(returnCode))
	{ }

	std::string Exception::GetErrorMessage(int returnCode)
	{
		char buf[128];
		snprintf(buf, sizeof(buf), "error 0x%08x", returnCode);
		return std::string(buf);
	}

}}
/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2016  Vladimir Menshakov

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

#include <Exception.h>
#include <string>

#include <string.h>
#include <errno.h>

namespace mtp { namespace posix
{

	Exception::Exception(const std::string &what) throw() : std::runtime_error(what + ": " + GetErrorMessage(errno))
	{ }

	Exception::Exception(const std::string &what, int returnCode) throw() : std::runtime_error(what + ": " + GetErrorMessage(returnCode))
	{ }

	std::string Exception::GetErrorMessage(int returnCode)
	{
		char buf[1024];
#if defined(__APPLE__) || ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE)
		int r = strerror_r(returnCode, buf, sizeof(buf));
		std::string text(r == 0? buf: "strerror_r() failed");
#else
		std::string text(strerror_r(returnCode, buf, sizeof(buf)));
#endif
		return text;
	}

}}
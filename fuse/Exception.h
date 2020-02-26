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

#ifndef AFTL_FUSE_EXCEPTION_H
#define AFTL_FUSE_EXCEPTION_H

#include <stdexcept>
#include <string>
#include <errno.h>
#include <string.h>

namespace mtp { namespace fuse
{

	class Exception : public std::runtime_error
	{
	public:
		Exception(const std::string &what) throw() : std::runtime_error(what + ": " + GetErrorMessage(errno)) { }
		Exception(const std::string &what, int returnCode) throw() : std::runtime_error(what + ": " + GetErrorMessage(returnCode)) { }
		static std::string GetErrorMessage(int returnCode)
		{
			char buf[1024];
#if defined(_GNU_SOURCE) && defined(__GLIBC__)
			std::string text(strerror_r(returnCode, buf, sizeof(buf)));
#else
			int r = strerror_r(returnCode, buf, sizeof(buf));
			std::string text(r == 0? buf: "strerror_r() failed");
#endif
			return text;
		}
	};

#define FUSE_CALL(...) do { int _r = __VA_ARGS__ ; if (_r < 0) throw mtp::fuse::Exception(#__VA_ARGS__ " failed", -_r); } while(false)

}}

#endif

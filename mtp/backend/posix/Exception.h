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

#ifndef AFTL_MTP_BACKEND_POSIX_EXCEPTION_H
#define AFTL_MTP_BACKEND_POSIX_EXCEPTION_H

#include <stdexcept>

namespace mtp { namespace posix
{
	class Exception : public std::runtime_error
	{
	public:
		Exception(const std::string &what) throw();
		Exception(const std::string &what, int retCode) throw();
		static std::string GetErrorMessage(int retCode);
	};
}}

#endif

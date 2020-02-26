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

#ifndef AFTL_MTP_DEMANGLE_H
#define AFTL_MTP_DEMANGLE_H

#include <cxxabi.h>
#include <stdlib.h>

namespace mtp
{
	//! demangles c++ symbol
	inline std::string Demangle(const char *abiName)
	{
		int status;
		char *ret = abi::__cxa_demangle(abiName, 0, 0, &status);

		std::string name(ret);
		free(ret);
		return name;
	}
}

#endif

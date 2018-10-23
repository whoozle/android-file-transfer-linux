/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2018  Vladimir Menshakov

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
#include <mtp/ptp/OutputStream.h>
#include <codecvt>
#include <locale>
#include <string>

namespace mtp
{
	void OutputStream::WriteString(const std::string &value)
	{
		if (value.empty())
		{
			Write8(0);
			return;
		}

		static std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
		std::wstring unicode = convert.from_bytes(value);

		size_t len = 1 + unicode.size();
		if (len > 255)
			throw std::runtime_error("string is too big (only 255 chars allowed, including null terminator)");

		Write8(len);
		for(auto uni : unicode)
		{
			Write16(uni < 0x10000? uni: 0xfffd); //replacement character
		}
		Write16(0);
	}
}
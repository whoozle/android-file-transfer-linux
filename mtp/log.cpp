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
#include <mtp/log.h>
#include <mtp/ptp/InputStream.h>

namespace mtp
{

	bool g_debug = false;

	void HexDump(std::stringstream & ss, const std::string &prefix, size_t size, InputStream & is)
	{
		ss << prefix << "[" << size << "]:\n";
		size_t i;

		std::string chars;
		chars.reserve(16);
		for(i = 0; i < size; ++i)
		{
			bool first = ((i & 0xf) == 0);
			bool last = ((i & 0xf) == 0x0f);
			if (first)
				ss << hex(i, 8) << ": ";

			u8 value = is.Read8();
			ss << hex(value, 2);
			chars.push_back(value < 0x20 || value >= 0x7f? '.': value);
			if (last)
			{
				ss << " " << chars << "\n";
				chars.clear();
			}
			else
				ss << " ";
		}
		if (chars.size())
		{
			ss << std::string((size_t)(16 - chars.size()) * 3, ' ') << chars << "\n";
		}
	}

	void HexDump(const std::string &prefix, const ByteArray &data, bool force)
	{
		if (!g_debug && !force)
			return;

		std::stringstream ss;
		InputStream is(data);
		HexDump(ss, prefix, data.size(), is);
		error(ss.str());
	}

}

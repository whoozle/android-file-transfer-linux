/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015  Vladimir Menshakov

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
#include <mtp/ByteArray.h>
#include <mtp/log.h>
#include <sstream>

namespace mtp
{

	void HexDump(const std::string &prefix, const ByteArray &data, bool force)
	{
		if (!g_debug && !force)
			return;

		std::stringstream ss;
		ss << prefix << "[" << data.size() << "]:\n";
		size_t i;

		std::string chars;
		chars.reserve(16);
		for(i = 0; i < data.size(); ++i)
		{
			bool first = ((i & 0xf) == 0);
			bool last = ((i & 0xf) == 0x0f);
			if (first)
				ss << hex(i, 8) << ": ";

			u8 value = data[i];
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

		error(ss.str());
	}
}

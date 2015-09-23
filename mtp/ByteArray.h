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

#ifndef BYTEARRAY_H
#define	BYTEARRAY_H

#include <mtp/types.h>
#include <vector>
#include <stdio.h>

namespace mtp
{
	typedef std::vector<u8> ByteArray;

	inline void HexDump(const std::string &prefix, const ByteArray &data)
	{
		fprintf(stderr, "%s[%lu]:\n", prefix.c_str(), (unsigned long)data.size());
		size_t i;
		for(i = 0; i < data.size(); ++i)
		{
			bool first = ((i & 0xf) == 0);
			bool last = ((i & 0xf) == 0x0f);
			if (first)
				fprintf(stderr, "%08lx: ", (unsigned long)i);
			u8 byte = data[i];
			fprintf(stderr, "%02x ", byte);
			if (last)
				fprintf(stderr, "\n");
		}
		if (i & 0xf)
			fprintf(stderr, "\n");
	}
}

#endif	/* BYTEARRAY_H */

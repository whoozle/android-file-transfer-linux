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
		printf("%s:\n", prefix.c_str());
		for(size_t i = 0; i < data.size(); ++i)
		{
			bool first = ((i & 0xf) == 0);
			bool last = ((i & 0xf) == 0x0f);
			if (first)
				printf("%08lx: ", (unsigned long)i);
			u8 byte = data[i];
			printf("%02x ", byte);
			if (last)
				printf("\n");
		}
		printf("\n");
	}
}

#endif	/* BYTEARRAY_H */

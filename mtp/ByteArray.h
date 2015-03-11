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
		printf("%s: ", prefix.c_str());
		for(size_t i = 0; i < data.size(); ++i)
		{
			u8 byte = data[i];
			printf("%02x ", byte);
		}
		printf("\n");
	}
}

#endif	/* BYTEARRAY_H */

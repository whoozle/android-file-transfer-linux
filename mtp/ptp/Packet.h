#ifndef PACKET_H
#define	PACKET_H

#include <mtp/types.h>
#include <vector>

namespace mtp
{
	struct Packet
	{
		std::vector<u8>		Data;

		void Write(u8 byte)
		{
			Data.push_back(byte);
		}

		void Write(u16 word)
		{
			Write((u8)word);
			Write((u8)(word >> 8));
		}

		void Write(u32 dword)
		{
			Write((u16)dword);
			Write((u16)(dword >> 16));
		}
	};
}

#endif	/* PACKET_H */

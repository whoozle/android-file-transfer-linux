#ifndef OPERATIONREQUEST_H
#define	OPERATIONREQUEST_H

#include <mtp/types.h>

namespace mtp
{
	enum class OperationCode : u16
	{
		GetDeviceInfo = 0x1001
	};

	struct OperationRequest
	{
		std::vector<u8>		Data;

		void Write(u8 byte)
		{
			Data.push_back(byte);
		}

		void Write(u16 word)
		{
			Data.push_back((u8)word);
			Data.push_back((u8)(word >> 8));
		}

		void Write(u32 dword)
		{
			Data.push_back((u16)dword);
			Data.push_back((u16)(dword >> 16));
		}

		OperationRequest(OperationCode opcode, u32 session = 0, u32 transaction = 0, u32 par1 = 0, u32 par2 = 0, u32 par3 = 0, u32 par4 = 0, u32 par5 = 0)
		{
			Data.reserve(30);
			Write((u16)opcode);
			Write(session);
			Write(transaction);
			Write(par1);
			Write(par2);
			Write(par3);
			Write(par4);
			Write(par5);
		}
	};
}

#endif
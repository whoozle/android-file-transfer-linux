#ifndef OPERATIONREQUEST_H
#define	OPERATIONREQUEST_H

#include <mtp/types.h>
#include <mtp/ptp/Packet.h>
#include <mtp/ptp/Container.h>

namespace mtp
{
	enum class OperationCode : u16
	{
		GetDeviceInfo = 0x1001
	};

	struct OperationRequest : Packet
	{
		static const ContainerType Type = ContainerType::Data;

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
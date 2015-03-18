#ifndef OPERATIONREQUEST_H
#define	OPERATIONREQUEST_H

#include <mtp/types.h>
#include <mtp/ptp/Container.h>
#include <mtp/ptp/OutputStream.h>
#include <mtp/ptp/OperationCode.h>

namespace mtp
{
	struct RequestBase
	{
		ByteArray					Data;

		RequestBase(OperationCode opcode, u32 transaction)
		{
			OutputStream stream(Data);
			stream << ((u16)opcode);
			//Write(session); //just one field in mtp
			stream << transaction;
		}

		void Append(const ByteArray &data)
		{
			std::copy(data.begin(), data.end(), std::back_inserter(Data));
		}
	};

	struct OperationRequest : RequestBase
	{
		static const ContainerType	Type = ContainerType::Command;

		OperationRequest(OperationCode opcode, u32 transaction):
			RequestBase(opcode, transaction)
		{ }
		OperationRequest(OperationCode opcode, u32 transaction, u32 par1):
			RequestBase(opcode, transaction)
		{
			OutputStream stream(Data);
			stream << par1;
		}
		OperationRequest(OperationCode opcode, u32 transaction, u32 par1, u32 par2):
			RequestBase(opcode, transaction)
		{
			OutputStream stream(Data);
			stream << par1;
			stream << par2;
		}
		OperationRequest(OperationCode opcode, u32 transaction, u32 par1, u32 par2, u32 par3):
			RequestBase(opcode, transaction)
		{
			OutputStream stream(Data);
			stream << par1;
			stream << par2;
			stream << par3;
		}
	};

	struct DataRequest : RequestBase
	{
		static const ContainerType	Type = ContainerType::Data;

		DataRequest(OperationCode opcode, u32 transaction): RequestBase(opcode, transaction) { }
	};
}

#endif
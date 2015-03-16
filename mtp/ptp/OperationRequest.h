#ifndef OPERATIONREQUEST_H
#define	OPERATIONREQUEST_H

#include <mtp/types.h>
#include <mtp/ptp/Container.h>
#include <mtp/ptp/OutputStream.h>

namespace mtp
{
	enum class OperationCode : u16
	{
		GetDeviceInfo			= 0x1001,
		OpenSession             = 0x1002,
		CloseSession            = 0x1003,
		GetStorageIDs           = 0x1004,
		GetStorageInfo          = 0x1005,
		GetNumObjects           = 0x1006,
		GetObjectHandles        = 0x1007,
		GetObjectInfo           = 0x1008,
		GetObject               = 0x1009,
		GetThumb                = 0x100a,
		DeleteObject            = 0x100b,
		SendObjectInfo          = 0x100c,
		SendObject              = 0x100d,
		InitiateCapture         = 0x100e,
		FormatStore             = 0x100f,
		ResetDevice             = 0x1010,
		SelfTest                = 0x1011,
		SetObjectProtection		= 0x1012,
		PowerDown               = 0x1013,
		GetDevicePropDesc       = 0x1014,
		GetDevicePropValue      = 0x1015,
		SetDevicePropValue      = 0x1016,
		ResetDevicePropValue    = 0x1017,
		TerminateOpenCapture    = 0x1018,
		MoveObject              = 0x1019,
		CopyObject              = 0x101a,
		GetPartialObject        = 0x101b,
		InitiateOpenCapture     = 0x101c
	};
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
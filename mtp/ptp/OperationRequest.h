#ifndef OPERATIONREQUEST_H
#define	OPERATIONREQUEST_H

#include <mtp/types.h>
#include <mtp/ptp/Packet.h>
#include <mtp/ptp/Container.h>

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

	struct OperationRequest : Packet
	{
		static const ContainerType	Type = ContainerType::Command;

		std::vector<u8>				Data;

		void Write(u8 byte)
		{ Data.push_back(byte); }

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
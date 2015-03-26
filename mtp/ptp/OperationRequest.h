/*
 * Android File Transfer for Linux: MTP client for android devices
 * Copyright (C) 2015  Vladimir Menshakov

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
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
			stream << opcode;
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
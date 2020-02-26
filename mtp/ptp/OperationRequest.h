/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2020  Vladimir Menshakov

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef AFTL_MTP_PTP_OPERATIONREQUEST_H
#define AFTL_MTP_PTP_OPERATIONREQUEST_H

#include <mtp/types.h>
#include <mtp/ptp/Container.h>
#include <mtp/ptp/OutputStream.h>
#include <mtp/ptp/OperationCode.h>

namespace mtp
{
	struct RequestBase //! base class for Operation and Data requests
	{
		ByteArray					Data; //!< resulting data

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

	struct OperationRequest : RequestBase //! Operation (Command) class, constructs operation request data in \ref Data
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
		OperationRequest(OperationCode opcode, u32 transaction, u32 par1, u32 par2, u32 par3, u32 par4):
			RequestBase(opcode, transaction)
		{
			OutputStream stream(Data);
			stream << par1;
			stream << par2;
			stream << par3;
			stream << par4;
		}
		OperationRequest(OperationCode opcode, u32 transaction, u32 par1, u32 par2, u32 par3, u32 par4, u32 par5):
			RequestBase(opcode, transaction)
		{
			OutputStream stream(Data);
			stream << par1;
			stream << par2;
			stream << par3;
			stream << par4;
			stream << par5;
		}
	};

	struct DataRequest : RequestBase //! MTP Data request
	{
		static const ContainerType	Type = ContainerType::Data;

		DataRequest(OperationCode opcode, u32 transaction): RequestBase(opcode, transaction) { }
	};
}

#endif
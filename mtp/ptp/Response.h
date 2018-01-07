/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2018  Vladimir Menshakov

    Android File Transfer For Linux is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    Android File Transfer For Linux is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Android File Transfer For Linux.
    If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RESPONSE_H
#define	RESPONSE_H

#include <mtp/ptp/OperationCode.h>
#include <stdexcept>

namespace mtp
{
	enum struct ContainerType : u16
	{
		Command = 1,
		Data = 2,
		Response = 3,
		Event = 4,
	};
	DECLARE_ENUM(ContainerType, u16);

	enum struct ResponseType : u16
	{
		OK							= 0x2001,
		GeneralError				= 0x2002,
		SessionNotOpen				= 0x2003,
		InvalidTransaction			= 0x2004,
		OperationNotSupported		= 0x2005,
		ParameterNotSupported		= 0x2006,
		IncompleteTransfer			= 0x2007,
		InvalidStorageID			= 0x2008,
		InvalidObjectHandle			= 0x2009,
		DevicePropNotSupported		= 0x200a,
		InvalidObjectFormatCode		= 0x200b,
		StoreFull					= 0x200c,
		ObjectWriteProtected		= 0x200d,
		StoreReadOnly				= 0x200e,
		AccessDenied				= 0x200f,
		NoThumbnailPresent			= 0x2010,
		SelfTestFailed				= 0x2011,
		PartialDeletion				= 0x2012,
		StoreNotAvailable			= 0x2013,
		SpecificationByFormatUnsupported = 0x2014,
		NoValidObjectInfo			= 0x2015,
		InvalidCodeFormat			= 0x2016,
		UnknownVendorCode			= 0x2017,
		CaptureAlreadyTerminated	= 0x2018,
		DeviceBusy					= 0x2019,
		InvalidParentObject			= 0x201a,
		InvalidDevicePropFormat		= 0x201b,
		InvalidDevicePropValue		= 0x201c,
		InvalidParameter			= 0x201d,
		SessionAlreadyOpen			= 0x201e,
		TransactionCancelled		= 0x201f,
		SpecificationOfDestinationUnsupported = 0x2020,

		InvalidObjectPropCode		= 0xa801,
		InvalidObjectPropFormat		= 0xa802,
		InvalidObjectPropValue		= 0xa803,
		InvalidObjectReference		= 0xa804,
		GroupNotSupported			= 0xa805,
		InvalidDataset				= 0xa806,
		UnsupportedSpecByGroup		= 0xa807,
		UnsupportedSpecByDepth		= 0xa808,
		ObjectTooLarge				= 0xa809,
		ObjectPropNotSupported		= 0xa80a
	};
	DECLARE_ENUM(ResponseType, u16);

	struct Response //! MTP Response class
	{
		static const size_t		Size = 8;

		mtp::ContainerType		ContainerType;
		mtp::ResponseType		ResponseType;
		u32						Transaction;

		Response() { }

		template<typename Stream>
		Response(Stream &stream)
		{ Read(stream); }

		template<typename Stream>
		void Read(Stream &stream)
		{
			stream >> ContainerType;
			stream >> ResponseType;
			stream >> Transaction;
		}
	};

	struct InvalidResponseException : public std::runtime_error //!Invalid MTP Response Exception
	{
		ResponseType Type;
		InvalidResponseException(const std::string &where, ResponseType type);
	};

}

#endif	/* RESPONSE_H */

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
	};
	DECLARE_ENUM(ResponseType, u16);

	struct Response
	{
		mtp::ContainerType		ContainerType;
		mtp::ResponseType		ResponseType;
		u32						Transaction;

		template<typename Stream>
		void Read(Stream &stream)
		{
			stream >> ContainerType;
			stream >> ResponseType;
			stream >> Transaction;
		}
	};

	struct InvalidResponseException : public std::runtime_error
	{
		InvalidResponseException(ResponseType type);
	};

}

#endif	/* RESPONSE_H */

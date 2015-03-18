#ifndef RESPONSE_H
#define	RESPONSE_H

namespace mtp
{
	enum struct ResponseType
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

}

#endif	/* RESPONSE_H */

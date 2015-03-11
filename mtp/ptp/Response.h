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
#if 0
		0x200A DeviceProp Not Supported
		0x200B Invalid ObjectFormatCode
		0x200C Store Full
		0x200D Object WriteProtected
		0x200E Store Read-Only
		0x200F Access Denied
		0x2010 No Thumbnail Present
		0x2011 SelfTest Failed
		0x2012 Partial Deletion
		0x2013 Store Not Available
		0x2014 Specification By Format Unsupported
		0x2015 No Valid ObjectInfo
		0x2016 Invalid Code Format
		0x2017 Unknown Vendor Code
		0x2018 Capture Already Terminated
		0x2019 Device Busy
		0x201A Invalid ParentObject
		0x201B Invalid DeviceProp Format
		0x201C Invalid DeviceProp Value
		0x201D Invalid Parameter
		0x201E Session Already Open
		0x201F Transaction Cancelled
		0x2020 Specification of Destination Unsupported
#endif
	};
}

#endif	/* RESPONSE_H */

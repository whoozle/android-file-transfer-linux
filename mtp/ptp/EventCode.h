#ifndef AFTL_PTP_EVENT_CODE
#define AFTL_PTP_EVENT_CODE

#include <mtp/types.h>

namespace mtp
{
	enum struct EventCode : u16
	{
		TransactionCancelled	= 0x4001,
		ObjectAdded				= 0x4002,
		ObjectRemoved			= 0x4003,
		StoreAdded				= 0x4004,
		StoreRemoved			= 0x4005,
		DevicePropChanged		= 0x4006,
		ObjectInfoChanged		= 0x4007,
		DeviceInfoChanged		= 0x4008,
		RequestObjectTransfer	= 0x4009,
		StoreFull				= 0x400a,
		DeviceReset				= 0x400b,
		StorageInfoChanged		= 0x400c,
		CaptureComplete			= 0x400d,
		UnreportedStatus		= 0x400e,

		ObjectPropChanged		= 0xc801,
		ObjectPropDescChanged	= 0xc802,
		ObjectReferenceChanged	= 0xc803
	};
	DECLARE_ENUM(EventCode, u16);
}

#endif

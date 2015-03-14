#ifndef PROTOCOL_H
#define	PROTOCOL_H

#include <mtp/usb/BulkPipe.h>
#include <mtp/ptp/Session.h>

namespace mtp
{
	class Device
	{
		PipePacketer	_packeter;

	public:
		Device(usb::BulkPipePtr pipe): _packeter(pipe) { }

		msg::DeviceInfo GetDeviceInfo();
		SessionPtr OpenSession(u32 sessionId);

		static DevicePtr Find(); //fixme: returns first device only
	};
}

#endif	/* PROTOCOL_H */


#ifndef PROTOCOL_H
#define	PROTOCOL_H

#include <mtp/usb/BulkPipe.h>
#include <mtp/ptp/Messages.h>

namespace mtp
{
	class PipePacketer
	{
		usb::BulkPipePtr	_pipe;

	public:
		PipePacketer(const usb::BulkPipePtr &pipe): _pipe(pipe) { }

		usb::BulkPipePtr GetPipe() const
		{ return _pipe; }

		void Write(const ByteArray &data);
		ByteArray Read();

	private:
		ByteArray ReadMessage();
	};

	class Session
	{
		PipePacketer	_packeter;
		u32				_sessionId;

	public:
		static const u32 Any = 0xffffffffu;

		Session(usb::BulkPipePtr pipe, u32 sessionId): _packeter(pipe), _sessionId(sessionId) { }

		msg::ObjectHandles GetObjectHandles(u32 storageId = Any, u32 objectFormat = 0);
		msg::StorageIDs GetStorageIDs();
		msg::StorageInfo GetStorageInfo(u32 storageId = Any, u32 formatCode = 0);
		msg::ObjectInfo GetObjectInfo(u32 objectId);
	};
	DECLARE_PTR(Session);


	class Device
	{
		PipePacketer	_packeter;

	public:
		Device(usb::BulkPipePtr pipe): _packeter(pipe) { }

		msg::DeviceInfo GetDeviceInfo();
		SessionPtr OpenSession(u32 sessionId);

	private:
	};
}

#endif	/* PROTOCOL_H */


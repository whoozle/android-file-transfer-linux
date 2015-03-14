#ifndef SESSION_H
#define	SESSION_H

#include <mtp/usb/BulkPipe.h>
#include <mtp/ptp/Messages.h>

namespace mtp
{
	class Device;
	DECLARE_PTR(Device);

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

	enum struct ObjectFormat
	{
		Undefined		= 0x3000,
		Association		= 0x3001,
		ExifJpeg		= 0x3801,
	};

	class Session
	{
		PipePacketer	_packeter;
		u32				_sessionId;
		u32				_transactionId;

	public:
		static const u32 AllStorages = 0xffffffffu;
		static const u32 Root = 0xffffffffu;
		static const u32 Device = 0;
		static const u32 AllFormats = 0;

		Session(usb::BulkPipePtr pipe, u32 sessionId): _packeter(pipe), _sessionId(sessionId), _transactionId(1) { }
		~Session() { try { Close(); } catch(const std::exception &ex) { } }

		msg::ObjectHandles GetObjectHandles(u32 storageId = AllStorages, u32 objectFormat = AllFormats, u32 parent = Device);
		msg::StorageIDs GetStorageIDs();
		msg::StorageInfo GetStorageInfo(u32 storageId);
		msg::ObjectInfo GetObjectInfo(u32 objectId);
		void DeleteObject(u32 objectId);

	private:
		void Close();
	};
	DECLARE_PTR(Session);

}

#endif	/* SESSION_H */


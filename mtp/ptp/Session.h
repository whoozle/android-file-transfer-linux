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

		void Write(const ByteArray &data, int timeout = 3000);
		void Read(u32 transaction, ByteArray &data, ByteArray &response, int timeout = 3000);

		void PollEvent();

	private:
		ByteArray ReadMessage(int timeout);
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

		struct NewObjectInfo
		{
			u32		StorageId;
			u32		ParentObjectId;
			u32		ObjectId;
		};

		Session(usb::BulkPipePtr pipe, u32 sessionId): _packeter(pipe), _sessionId(sessionId), _transactionId(1) { }
		~Session() { try { Close(); } catch(const std::exception &ex) { } }

		msg::ObjectHandles GetObjectHandles(u32 storageId = AllStorages, u32 objectFormat = AllFormats, u32 parent = Device);
		msg::StorageIDs GetStorageIDs();
		msg::StorageInfo GetStorageInfo(u32 storageId);
		msg::ObjectInfo GetObjectInfo(u32 objectId);
		ByteArray GetObject(u32 objectId);
		NewObjectInfo SendObjectInfo(const msg::ObjectInfo &objectInfo, u32 storageId = 0, u32 parentObject = 0);
		void SendObject(const ByteArray &array);
		void DeleteObject(u32 objectId);
		msg::ObjectPropsSupported GetObjectPropsSupported(u32 objectId);

	private:
		void Close();
	};
	DECLARE_PTR(Session);

}

#endif	/* SESSION_H */


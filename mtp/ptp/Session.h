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
		void Read(ByteArray &data, ByteArray &response, int timeout = 3000);

	private:
		ByteArray ReadMessage(int timeout);
	};

	enum struct ObjectFormat : u16
	{
		Undefined		= 0x3000,
		Association		= 0x3001,
		Script			= 0x3002,
		Executable		= 0x3003,
		Text			= 0x3004,
		Html			= 0x3005,
		Dpof			= 0x3006,
		Aiff			= 0x3007,
		Wav				= 0x3008,
		Mp3				= 0x3009,
		Avi				= 0x300a,
		Mpeg			= 0x300b,
		Asf				= 0x300c,
		UndefinedImage	= 0x3800,
		ExifJpeg		= 0x3801,
		TiffEp			= 0x3802,
		Flashpix		= 0x3803,
		Bmp				= 0x3804,
		Ciff			= 0x3805,
		Reserved		= 0x3806,
		Gif				= 0x3807,
		Jfif			= 0x3808,
		Pcd				= 0x3809,
		Pict			= 0x380a,
		Png				= 0x380b,
		Reserved2		= 0x380c,
		Tiff			= 0x380d,
		TiffIt			= 0x380e,
		Jp2				= 0x380f,
		Jpx				= 0x3810,
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

	private:
		void Close();
	};
	DECLARE_PTR(Session);

}

#endif	/* SESSION_H */


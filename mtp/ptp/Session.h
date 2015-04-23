/*
 * Android File Transfer for Linux: MTP client for android devices
 * Copyright (C) 2015  Vladimir Menshakov

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef SESSION_H
#define	SESSION_H

#include <mtp/usb/BulkPipe.h>
#include <mtp/ptp/Messages.h>
#include <mtp/ptp/PipePacketer.h>
#include <mtp/ptp/DeviceProperty.h>
#include <mtp/ptp/ObjectProperty.h>
#include <mtp/ptp/IObjectStream.h>

namespace mtp
{
	struct OperationRequest;

	class Session;
	DECLARE_PTR(Session);

	class Session
	{
		std::mutex		_mutex;
		PipePacketer	_packeter;
		u32				_sessionId;
		u32				_transactionId;

		bool			_supportedGetPartialObject64;

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

		///handles partial writes and
		class ObjectEditSession : Noncopyable
		{
			SessionPtr	_session;
			u32			_objectId;

		public:
			ObjectEditSession(const SessionPtr & session, u32 objectId);
			~ObjectEditSession();

			void Truncate(u64 size);
			void Send(u64 offset, const ByteArray &data);
		};
		DECLARE_PTR(ObjectEditSession);

		Session(usb::BulkPipePtr pipe, u32 sessionId, const msg::DeviceInfo &deviceInfo);
		~Session();

		msg::ObjectHandles GetObjectHandles(u32 storageId = AllStorages, u32 objectFormat = AllFormats, u32 parent = Device);
		msg::StorageIDs GetStorageIDs();
		msg::StorageInfo GetStorageInfo(u32 storageId);

		msg::ObjectInfo GetObjectInfo(u32 objectId);
		void GetObject(u32 objectId, const IObjectOutputStreamPtr &outputStream);
		ByteArray GetPartialObject(u32 objectId, u64 offset, u32 size);
		NewObjectInfo SendObjectInfo(const msg::ObjectInfo &objectInfo, u32 storageId = 0, u32 parentObject = 0);
		void SendObject(const IObjectInputStreamPtr &inputStream);
		void DeleteObject(u32 objectId);

		static ObjectEditSessionPtr EditObject(const SessionPtr &session, u32 objectId)
		{ return std::make_shared<ObjectEditSession>(session, objectId); }

		msg::ObjectPropsSupported GetObjectPropsSupported(u32 objectId);

		void SetObjectProperty(u32 objectId, ObjectProperty property, const ByteArray &value);
		void SetObjectProperty(u32 objectId, ObjectProperty property, const std::string &value);
		ByteArray GetObjectProperty(u32 objectId, ObjectProperty property);
		u64 GetObjectIntegerProperty(u32 objectId, ObjectProperty property);
		std::string GetObjectStringProperty(u32 objectId, ObjectProperty property);

		ByteArray GetDeviceProperty(DeviceProperty property);

	private:
		void BeginEditObject(u32 objectId);
		void SendPartialObject(u32 objectId, u64 offset, const ByteArray &data);
		void TruncateObject(u32 objectId, u64 size);
		void EndEditObject(u32 objectId);

		ByteArray Get(u32 transaction);
		void Send(const OperationRequest &req);
		void Close();
	};

}

#endif	/* SESSION_H */


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
		void GetObject(u32 objectId, const IObjectOutputStreamPtr &outputStream);
		NewObjectInfo SendObjectInfo(const msg::ObjectInfo &objectInfo, u32 storageId = 0, u32 parentObject = 0);
		void SendObject(const IObjectInputStreamPtr &inputStream);
		void DeleteObject(u32 objectId);

		msg::ObjectPropsSupported GetObjectPropsSupported(u32 objectId);

		void SetObjectProperty(u32 objectId, ObjectProperty property, const ByteArray &value);
		void SetObjectProperty(u32 objectId, ObjectProperty property, const std::string &value);
		ByteArray GetObjectProperty(u32 objectId, ObjectProperty property);

		ByteArray GetDeviceProperty(DeviceProperty property);

	private:
		void Close();
	};
	DECLARE_PTR(Session);

}

#endif	/* SESSION_H */


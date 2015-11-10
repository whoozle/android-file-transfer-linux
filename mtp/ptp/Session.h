/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015  Vladimir Menshakov

    Android File Transfer For Linux is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    Android File Transfer For Linux is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Android File Transfer For Linux.
    If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SESSION_H
#define	SESSION_H

#include <mtp/usb/BulkPipe.h>
#include <mtp/ptp/Messages.h>
#include <mtp/ptp/DeviceProperty.h>
#include <mtp/ptp/IObjectStream.h>
#include <mtp/ptp/ObjectId.h>
#include <mtp/ptp/ObjectProperty.h>
#include <mtp/ptp/PipePacketer.h>

namespace mtp
{
	struct OperationRequest;

	class Session;
	DECLARE_PTR(Session);

	class Session
	{
		class Transaction;

		std::mutex		_mutex, _transactionMutex;
		PipePacketer	_packeter;
		u32				_sessionId;
		u32				_nextTransactionId;
		Transaction *	_transaction;

		msg::DeviceInfo	_deviceInfo;
		bool			_getPartialObject64Supported;
		bool			_editObjectSupported;
		bool			_getObjectPropertyListSupported;
		int				_defaultTimeout;

	public:
		static const StorageId AllStorages;
		static const StorageId AnyStorage;
		static const ObjectId Device;
		static const ObjectId Root;

		struct NewObjectInfo
		{
			mtp::StorageId		StorageId;
			mtp::ObjectId		ParentObjectId;
			mtp::ObjectId		ObjectId;
		};

		///handles partial writes and
		class ObjectEditSession : Noncopyable
		{
			SessionPtr	_session;
			ObjectId	_objectId;

		public:
			ObjectEditSession(const SessionPtr & session, ObjectId objectId);
			~ObjectEditSession();

			void Truncate(u64 size);
			void Send(u64 offset, const ByteArray &data);
		};
		DECLARE_PTR(ObjectEditSession);

		Session(usb::BulkPipePtr pipe, u32 sessionId);
		~Session();

		const msg::DeviceInfo & GetDeviceInfo() const
		{ return _deviceInfo; }

		msg::ObjectHandles GetObjectHandles(StorageId storageId = AllStorages, ObjectFormat objectFormat = ObjectFormat::Any, ObjectId parent = Device, int timeout = 30000);
		msg::StorageIDs GetStorageIDs();
		msg::StorageInfo GetStorageInfo(StorageId storageId);

		NewObjectInfo CreateDirectory(const std::string &name, ObjectId parentId, StorageId storageId = AnyStorage, AssociationType type = AssociationType::GenericFolder);
		msg::ObjectInfo GetObjectInfo(ObjectId objectId);
		void GetObject(ObjectId objectId, const IObjectOutputStreamPtr &outputStream);
		ByteArray GetPartialObject(ObjectId objectId, u64 offset, u32 size);
		NewObjectInfo SendObjectInfo(const msg::ObjectInfo &objectInfo, StorageId storageId = AnyStorage, ObjectId parentObject = Device);
		void SendObject(const IObjectInputStreamPtr &inputStream, int timeout = 10000);
		void DeleteObject(ObjectId objectId);

		bool EditObjectSupported() const
		{ return _editObjectSupported; }
		bool GetObjectPropertyListSupported() const
		{ return _getObjectPropertyListSupported; }

		static ObjectEditSessionPtr EditObject(const SessionPtr &session, ObjectId objectId)
		{ return std::make_shared<ObjectEditSession>(session, objectId); }

		msg::ObjectPropsSupported GetObjectPropsSupported(ObjectId objectId);

		void SetObjectProperty(ObjectId objectId, ObjectProperty property, const ByteArray &value);
		void SetObjectProperty(ObjectId objectId, ObjectProperty property, u64 value);
		void SetObjectProperty(ObjectId objectId, ObjectProperty property, const std::string &value);

		//common properties shortcuts
		StorageId GetObjectStorage(ObjectId id);
		ObjectId GetObjectParent(ObjectId id);

		ByteArray GetObjectProperty(ObjectId objectId, ObjectProperty property);
		u64 GetObjectIntegerProperty(ObjectId objectId, ObjectProperty property);
		std::string GetObjectStringProperty(ObjectId objectId, ObjectProperty property);

		ByteArray GetObjectPropertyList(ObjectId objectId, ObjectFormat format, ObjectProperty property, u32 groupCode, u32 depth);

		ByteArray GetDeviceProperty(DeviceProperty property);

		void AbortCurrentTransaction(int timeout);

	private:
		void SetCurrentTransaction(Transaction *);

		msg::DeviceInfo GetDeviceInfoImpl();

		void BeginEditObject(ObjectId objectId);
		void SendPartialObject(ObjectId objectId, u64 offset, const ByteArray &data);
		void TruncateObject(ObjectId objectId, u64 size);
		void EndEditObject(ObjectId objectId);

		ByteArray Get(u32 transaction, int timeout = 0);
		void Send(const OperationRequest &req, int timeout = 0);
		void Close();
	};

}

#endif	/* SESSION_H */


/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2020  Vladimir Menshakov

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef AFTL_MTP_PTP_SESSION_H
#define AFTL_MTP_PTP_SESSION_H

#include <mtp/usb/BulkPipe.h>
#include <mtp/ptp/Messages.h>
#include <mtp/ptp/DeviceProperty.h>
#include <mtp/ptp/IObjectStream.h>
#include <mtp/ptp/ObjectId.h>
#include <mtp/ptp/ObjectProperty.h>
#include <mtp/ptp/PipePacketer.h>
#include <time.h>

namespace mtp
{
	struct OperationRequest;

	class Session;
	DECLARE_PTR(Session);

	class Session //! Main MTP interaction / object manipulation class
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
		bool			_getObjectPropValueSupported;
		bool			_getObjectModificationTimeBuggy;
		bool			_separateBulkWrites;
		int				_defaultTimeout;

	public:
		static constexpr int DefaultTimeout		= 10000;
		static constexpr int LongTimeout		= 30000;

		static const StorageId AllStorages;
		static const StorageId AnyStorage;
		static const ObjectId Device;
		static const ObjectId Root;

		///sub-session object which handles partial writes and truncation
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

		Session(const PipePacketer & packeter, u32 sessionId);
		~Session();

		const msg::DeviceInfo & GetDeviceInfo() const
		{ return _deviceInfo; }

		msg::ObjectHandles GetObjectHandles(StorageId storageId = AllStorages, ObjectFormat objectFormat = ObjectFormat::Any, ObjectId parent = Device, int timeout = LongTimeout);
		msg::StorageIDs GetStorageIDs();
		msg::StorageInfo GetStorageInfo(StorageId storageId);

		msg::NewObjectInfo CreateDirectory(const std::string &name, ObjectId parentId, StorageId storageId = AnyStorage, AssociationType type = AssociationType::GenericFolder);
		msg::ObjectInfo GetObjectInfo(ObjectId objectId);
		void GetObject(ObjectId objectId, const IObjectOutputStreamPtr &outputStream);
		void GetThumb(ObjectId objectId, const IObjectOutputStreamPtr &outputStream);
		ByteArray GetPartialObject(ObjectId objectId, u64 offset, u32 size);
		msg::NewObjectInfo SendObjectInfo(const msg::ObjectInfo &objectInfo, StorageId storageId = AnyStorage, ObjectId parentObject = Device);
		void SendObject(const IObjectInputStreamPtr &inputStream, int timeout = LongTimeout);
		void DeleteObject(ObjectId objectId, int timeout = LongTimeout);

		bool EditObjectSupported() const
		{ return _editObjectSupported; }
		bool GetObjectPropertyListSupported() const
		{ return _getObjectPropertyListSupported; }

		static ObjectEditSessionPtr EditObject(const SessionPtr &session, ObjectId objectId)
		{ return std::make_shared<ObjectEditSession>(session, objectId); }

		msg::ObjectPropertiesSupported GetObjectPropertiesSupported(ObjectFormat format);
		ByteArray GetObjectPropertyDesc(ObjectProperty code, ObjectFormat format);

		void SetObjectProperty(ObjectId objectId, ObjectProperty property, const ByteArray &value);
		void SetObjectProperty(ObjectId objectId, ObjectProperty property, u64 value);
		void SetObjectProperty(ObjectId objectId, ObjectProperty property, const std::string &value);
		void SetObjectPropertyAsArray(ObjectId objectId, ObjectProperty property, const ByteArray &value);
		time_t GetObjectModificationTime(ObjectId id);

		//common properties shortcuts
		StorageId GetObjectStorage(ObjectId id);
		ObjectId GetObjectParent(ObjectId id);

		ByteArray GetObjectProperty(ObjectId objectId, ObjectProperty property);
		u64 GetObjectIntegerProperty(ObjectId objectId, ObjectProperty property);
		std::string GetObjectStringProperty(ObjectId objectId, ObjectProperty property);

		ByteArray GetObjectPropertyList(ObjectId objectId, ObjectFormat format, ObjectProperty property, u32 groupCode, u32 depth, int timeout = LongTimeout);
		msg::SendObjectPropListResponse SendObjectPropList(StorageId storageId, ObjectId parentId, ObjectFormat format, u64 objectSize, const ByteArray & propList);

		msg::DevicePropertyDesc GetDevicePropertyDesc(DeviceProperty property);
		ByteArray GetDeviceProperty(DeviceProperty property);
		u64 GetDeviceIntegerProperty(DeviceProperty property);
		std::string GetDeviceStringProperty(DeviceProperty property);
		void SetDeviceProperty(DeviceProperty property, const ByteArray & value);
		void SetDeviceProperty(DeviceProperty property, const std::string & value);

		void AbortCurrentTransaction(int timeout = DefaultTimeout);

		ByteArray GenericOperation(OperationCode code);
		ByteArray GenericOperation(OperationCode code, const ByteArray & payload);

		void SetObjectReferences(ObjectId objectId, const msg::ObjectHandles &objects);
		msg::ObjectHandles GetObjectReferences(ObjectId objectId);

		//windows specific
		void EnableSecureFileOperations(u32 cmac1[4]);
		void RebootDevice();

		static msg::DeviceInfo GetDeviceInfo(PipePacketer& packeter, u32 transactionId, int timeout = 0);

	private:
		template<typename ... Args>
		ByteArray RunTransaction(int timeout, OperationCode code, Args && ... args)
		{ ByteArray response; return RunTransactionWithDataRequest<Args...>(timeout, code, response, nullptr, std::forward<Args>(args) ... ); }
		template<typename ... Args>
		ByteArray RunTransactionWithDataRequest(int timeout, OperationCode code, ByteArray & response, const IObjectInputStreamPtr & inputStream, Args && ... args);

		void SetCurrentTransaction(Transaction *);


		void BeginEditObject(ObjectId objectId);
		void SendPartialObject(ObjectId objectId, u64 offset, const ByteArray &data);
		void TruncateObject(ObjectId objectId, u64 size);
		void EndEditObject(ObjectId objectId);

		ByteArray Get(u32 transaction, ByteArray & response, int timeout = 0);
		static ByteArray Get(PipePacketer &packeter, u32 transaction, ByteArray & response, int timeout = 0);

		void Send(const OperationRequest &req, int timeout = 0);
		static void Send(PipePacketer &packeter, const OperationRequest &req, int timeout = 0);
		void Close();
	};

}

#endif	/* SESSION_H */


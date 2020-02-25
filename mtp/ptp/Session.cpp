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

#include <mtp/ptp/Session.h>
#include <mtp/ptp/Messages.h>
#include <mtp/ptp/Container.h>
#include <mtp/ptp/OperationRequest.h>
#include <mtp/ptp/ByteArrayObjectStream.h>
#include <mtp/ptp/JoinedObjectStream.h>
#include <mtp/log.h>
#include <usb/Device.h>
#include <limits>
#include <array>

namespace mtp
{

	const StorageId Session::AnyStorage(0);
	const StorageId Session::AllStorages(0xffffffffu);
	const ObjectId Session::Device(0);
	const ObjectId Session::Root(0xffffffffu);


#define CHECK_RESPONSE(RCODE) do { \
	if ((RCODE) != ResponseType::OK && (RCODE) != ResponseType::SessionAlreadyOpen) \
		throw InvalidResponseException(__func__, (RCODE)); \
} while(false)

	Session::Session(usb::BulkPipePtr pipe, u32 sessionId):
		_packeter(pipe), _sessionId(sessionId), _nextTransactionId(1), _transaction(),
		_getObjectModificationTimeBuggy(false),
		_defaultTimeout(DefaultTimeout)
	{
		_deviceInfo = GetDeviceInfoImpl();
		_getPartialObject64Supported = _deviceInfo.Supports(OperationCode::GetPartialObject64);
		_getObjectPropertyListSupported = _deviceInfo.Supports(OperationCode::GetObjectPropList);
		_editObjectSupported = _deviceInfo.Supports(OperationCode::BeginEditObject) &&
			_deviceInfo.Supports(OperationCode::EndEditObject) &&
			_deviceInfo.Supports(OperationCode::TruncateObject) &&
			_deviceInfo.Supports(OperationCode::SendPartialObject);
	}

	Session::~Session()
	{ try { Close(); } catch(const std::exception &ex) { } }

	class Session::Transaction
	{
		Session *	_session;
	public:
		u32			Id;

		Transaction(Session *session): _session(session)
		{ session->SetCurrentTransaction(this); }
		~Transaction()
		{ _session->SetCurrentTransaction(0); }
	};

	void Session::SetCurrentTransaction(Transaction *transaction)
	{
		scoped_mutex_lock l(_transactionMutex);
		_transaction = transaction;
		if (_transaction)
			_transaction->Id = _nextTransactionId++;
	}


	void Session::Send(const OperationRequest &req, int timeout)
	{
		if (timeout <= 0)
			timeout = _defaultTimeout;
		Container container(req);
		_packeter.Write(container.Data, timeout);
	}

	void Session::Close()
	{
		scoped_mutex_lock l(_mutex);
		Send(OperationRequest(OperationCode::CloseSession, 0, _sessionId));
		ByteArray data, response;
		ResponseType responseCode;
		_packeter.Read(0, data, responseCode, response, _defaultTimeout);
		//HexDump("payload", data);
	}

	ByteArray Session::Get(u32 transaction, int timeout)
	{
		if (timeout <= 0)
			timeout = _defaultTimeout;
		ByteArray data, response;
		ResponseType responseCode;
		_packeter.Read(transaction, data, responseCode, response, timeout);
		CHECK_RESPONSE(responseCode);
		return data;
	}

	template<typename ... Args>
	ByteArray Session::RunTransactionWithDataRequest(int timeout, OperationCode code, const IObjectInputStreamPtr & inputStream, Args && ... args)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(code, transaction.Id, std::forward<Args>(args) ... ), timeout);
		if (inputStream)
		{
			DataRequest req(code, transaction.Id);
			Container container(req, inputStream);
			_packeter.Write(std::make_shared<ByteArrayObjectInputStream>(container.Data), timeout);
			_packeter.Write(inputStream, timeout);
		}
		return Get(transaction.Id);
	}

	msg::DeviceInfo Session::GetDeviceInfoImpl()
	{
		auto data = RunTransaction(_defaultTimeout, OperationCode::GetDeviceInfo);
		InputStream stream(data); //operation code + session id
		msg::DeviceInfo gdi;
		gdi.Read(stream);
		return gdi;
	}


	msg::ObjectHandles Session::GetObjectHandles(StorageId storageId, ObjectFormat objectFormat, ObjectId parent, int timeout)
	{
		auto data = RunTransaction(timeout, OperationCode::GetObjectHandles, storageId.Id, static_cast<u32>(objectFormat), parent.Id);
		InputStream stream(data);

		msg::ObjectHandles goh;
		goh.Read(stream);
		return goh;
	}

	msg::StorageIDs Session::GetStorageIDs()
	{
		auto data = RunTransaction(_defaultTimeout, OperationCode::GetStorageIDs);
		InputStream stream(data);

		msg::StorageIDs gsi;
		gsi.Read(stream);
		return gsi;
	}

	msg::StorageInfo Session::GetStorageInfo(StorageId storageId)
	{
		auto data = RunTransaction(_defaultTimeout, OperationCode::GetStorageInfo, storageId.Id);
		InputStream stream(data);
		msg::StorageInfo gsi;
		gsi.Read(stream);
		return gsi;
	}

	Session::NewObjectInfo Session::CreateDirectory(const std::string &name, ObjectId parentId, StorageId storageId, AssociationType type)
	{
		mtp::msg::ObjectInfo oi;
		oi.Filename = name;
		oi.ParentObject = parentId;
		oi.StorageId = storageId;
		oi.ObjectFormat = mtp::ObjectFormat::Association;
		oi.AssociationType = type;
		mtp::Session::NewObjectInfo noi = SendObjectInfo(oi, storageId, parentId);

		//SendObject(std::make_shared<ByteArrayObjectInputStream>(ByteArray()));
		return noi;
	}

	msg::ObjectInfo Session::GetObjectInfo(ObjectId objectId)
	{
		auto data = RunTransaction(_defaultTimeout, OperationCode::GetObjectInfo, objectId.Id);
		InputStream stream(data);
		msg::ObjectInfo goi;
		goi.Read(stream);
		return goi;
	}

	msg::ObjectPropertiesSupported Session::GetObjectPropertiesSupported(ObjectFormat format)
	{
		auto data = RunTransaction(_defaultTimeout, OperationCode::GetObjectPropsSupported, static_cast<u32>(format));
		InputStream stream(data);
		msg::ObjectPropertiesSupported ops;
		ops.Read(stream);
		return ops;
	}

	ByteArray Session::GetObjectPropertyDesc(ObjectProperty code)
	{ return RunTransaction(_defaultTimeout, OperationCode::GetObjectPropDesc, static_cast<u32>(code)); }

	void Session::GetObject(ObjectId objectId, const IObjectOutputStreamPtr &outputStream)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetObject, transaction.Id, objectId.Id));
		ByteArray response;
		ResponseType responseCode;
		_packeter.Read(transaction.Id, outputStream, responseCode, response, _defaultTimeout);
		CHECK_RESPONSE(responseCode);
	}

	void Session::GetThumb(ObjectId objectId, const IObjectOutputStreamPtr &outputStream)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetThumb, transaction.Id, objectId.Id));
		ByteArray response;
		ResponseType responseCode;
		_packeter.Read(transaction.Id, outputStream, responseCode, response, _defaultTimeout);
		CHECK_RESPONSE(responseCode);
	}

	ByteArray Session::GetPartialObject(ObjectId objectId, u64 offset, u32 size)
	{
		if (_getPartialObject64Supported)
			return RunTransaction(_defaultTimeout, OperationCode::GetPartialObject64, objectId.Id, offset, offset >> 32, size);
		else
		{
			if (offset + size > std::numeric_limits<u32>::max())
				throw std::runtime_error("32 bit overflow for GetPartialObject");
			return RunTransaction(_defaultTimeout, OperationCode::GetPartialObject, objectId.Id, offset, size);
		}
	}


	Session::NewObjectInfo Session::SendObjectInfo(const msg::ObjectInfo &objectInfo, StorageId storageId, ObjectId parentObject)
	{
		if (objectInfo.Filename.empty())
			throw std::runtime_error("object filename must not be empty");
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::SendObjectInfo, transaction.Id, storageId.Id, parentObject.Id));
		{
			DataRequest req(OperationCode::SendObjectInfo, transaction.Id);
			OutputStream stream(req.Data);
			objectInfo.Write(stream);
			Container container(req);
			_packeter.Write(container.Data, _defaultTimeout);
		}
		ByteArray data, response;
		ResponseType responseCode;
		_packeter.Read(transaction.Id, data, responseCode, response, _defaultTimeout);
		//HexDump("response", response);
		CHECK_RESPONSE(responseCode);
		InputStream stream(response);
		NewObjectInfo noi;
		stream >> noi.StorageId;
		stream >> noi.ParentObjectId;
		stream >> noi.ObjectId;
		return noi;
	}

	void Session::SendObject(const IObjectInputStreamPtr &inputStream, int timeout)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::SendObject, transaction.Id));
		{
			DataRequest req(OperationCode::SendObject, transaction.Id);
			Container container(req, inputStream);
			_packeter.Write(std::make_shared<JoinedObjectInputStream>(std::make_shared<ByteArrayObjectInputStream>(container.Data), inputStream), timeout);
		}
		Get(transaction.Id);
	}

	void Session::BeginEditObject(ObjectId objectId)
	{ RunTransaction(_defaultTimeout, OperationCode::BeginEditObject, objectId.Id); }

	void Session::SendPartialObject(ObjectId objectId, u64 offset, const ByteArray &data)
	{
		IObjectInputStreamPtr inputStream = std::make_shared<ByteArrayObjectInputStream>(data);
		RunTransactionWithDataRequest(_defaultTimeout, OperationCode::SendPartialObject, inputStream, objectId.Id, offset, offset >> 32, data.size());
	}

	void Session::TruncateObject(ObjectId objectId, u64 size)
	{ RunTransaction(_defaultTimeout, OperationCode::TruncateObject, objectId.Id, size, size >> 32); }

	void Session::EndEditObject(ObjectId objectId)
	{ RunTransaction(_defaultTimeout, OperationCode::EndEditObject, objectId.Id); }

	void Session::SetObjectProperty(ObjectId objectId, ObjectProperty property, const ByteArray &value)
	{
		IObjectInputStreamPtr inputStream = std::make_shared<ByteArrayObjectInputStream>(value);
		RunTransactionWithDataRequest(_defaultTimeout, OperationCode::SetObjectPropValue, inputStream, objectId.Id, (u16)property);
	}

	StorageId Session::GetObjectStorage(mtp::ObjectId id)
	{
		StorageId storageId(GetObjectIntegerProperty(id, ObjectProperty::StorageId));
		if (storageId == AnyStorage || storageId == AllStorages)
			throw std::runtime_error("returned wildcard storage id as storage for object");
		return storageId;
	}

	ObjectId Session::GetObjectParent(mtp::ObjectId id)
	{ return ObjectId(GetObjectIntegerProperty(id, ObjectProperty::ParentObject)); }

	void Session::SetObjectProperty(ObjectId objectId, ObjectProperty property, const std::string &value)
	{
		ByteArray data;
		data.reserve(value.size() * 2 + 1);
		OutputStream stream(data);
		stream << value;
		SetObjectProperty(objectId, property, data);
	}

	ByteArray Session::GetObjectProperty(ObjectId objectId, ObjectProperty property)
	{ return RunTransaction(_defaultTimeout, OperationCode::GetObjectPropValue, objectId.Id, (u16)property); }

	u64 Session::GetObjectIntegerProperty(ObjectId objectId, ObjectProperty property)
	{ return ReadSingleInteger(GetObjectProperty(objectId, property)); }

	void Session::SetObjectProperty(ObjectId objectId, ObjectProperty property, u64 value)
	{
		std::array<u8, sizeof(value)> data;
		std::fill(data.begin(), data.end(), 0);
		size_t i;
		for(i = 0; i < data.size() && value != 0; ++i, value >>= 8)
		{
			data[i] = value;
		}
		if (i <= 4)
			i = 4;
		else
			i = 8;

		SetObjectProperty(objectId, property, ByteArray(data.begin(), data.begin() + i));
	}

	std::string Session::GetObjectStringProperty(ObjectId objectId, ObjectProperty property)
	{
		return ReadSingleString(GetObjectProperty(objectId, property));
	}

	time_t Session::GetObjectModificationTime(ObjectId id)
	{
		if (!_getObjectModificationTimeBuggy)
		{
			try
			{
				auto mtimeStr = GetObjectStringProperty(id, mtp::ObjectProperty::DateModified);
				auto mtime = mtp::ConvertDateTime(mtimeStr);
				if (mtime != 0) //long standing Android bug
					return mtime;
			}
			catch(const std::exception &ex)
			{
				debug("exception while getting mtime: ", ex.what());
			}
			_getObjectModificationTimeBuggy = true;
		}
		auto oi = GetObjectInfo(id);
		return mtp::ConvertDateTime(oi.ModificationDate);
	}

	u64 Session::GetDeviceIntegerProperty(DeviceProperty property)
	{ return ReadSingleInteger(GetDeviceProperty(property)); }

	std::string Session::GetDeviceStringProperty(DeviceProperty property)
	{ return ReadSingleString(GetDeviceProperty(property)); }

	void Session::SetDeviceProperty(DeviceProperty property, const ByteArray &value)
	{
		IObjectInputStreamPtr inputStream = std::make_shared<ByteArrayObjectInputStream>(value);
		RunTransactionWithDataRequest(_defaultTimeout, OperationCode::SetDevicePropValue, inputStream, (u16)property);
	}

	void Session::SetDeviceProperty(DeviceProperty property, const std::string &value)
	{
		ByteArray data;
		data.reserve(value.size() * 2 + 1);
		OutputStream stream(data);
		stream << value;
		SetDeviceProperty(property, data);
	}


	ByteArray Session::GetObjectPropertyList(ObjectId objectId, ObjectFormat format, ObjectProperty property, u32 groupCode, u32 depth, int timeout)
	{
		if (objectId == Root) //ffffffff -> 0
			objectId = Device;
		return RunTransaction(timeout, OperationCode::GetObjectPropList, objectId.Id, (u32)format, property != ObjectProperty::All? (u32)property: 0xffffffffu, groupCode, depth);
	}

	void Session::DeleteObject(ObjectId objectId, int timeout)
	{ RunTransaction(timeout, OperationCode::DeleteObject, objectId.Id, 0); }

	ByteArray Session::GetDeviceProperty(DeviceProperty property)
	{ return RunTransaction(_defaultTimeout, OperationCode::GetDevicePropValue, (u16)property); }

	ByteArray Session::GenericOperation(OperationCode code)
	{ return RunTransaction(_defaultTimeout, code); }

	ByteArray Session::GenericOperation(OperationCode code, const ByteArray & payload)
	{
		IObjectInputStreamPtr inputStream = std::make_shared<ByteArrayObjectInputStream>(payload);
		return RunTransactionWithDataRequest(_defaultTimeout, code, inputStream);
	}

	void Session::EnableSecureFileOperations(u32 cmac[4])
	{ RunTransaction(_defaultTimeout, OperationCode::EnableTrustedFilesOperations, cmac[0], cmac[1], cmac[2], cmac[3]); }

	Session::ObjectEditSession::ObjectEditSession(const SessionPtr & session, ObjectId objectId): _session(session), _objectId(objectId)
	{ session->BeginEditObject(objectId); }

	Session::ObjectEditSession::~ObjectEditSession()
	{ _session->EndEditObject(_objectId); }

	void Session::ObjectEditSession::Truncate(u64 size)
	{ _session->TruncateObject(_objectId, size); }

	void Session::ObjectEditSession::Send(u64 offset, const ByteArray &data)
	{ _session->SendPartialObject(_objectId, offset, data); }

	void Session::AbortCurrentTransaction(int timeout)
	{
		u32 transactionId;
		{
			scoped_mutex_lock l(_transactionMutex);
			if (!_transaction)
				throw std::runtime_error("no transaction in progress");
			transactionId = _transaction->Id;
		}
		_packeter.Abort(transactionId, timeout);
	}

}

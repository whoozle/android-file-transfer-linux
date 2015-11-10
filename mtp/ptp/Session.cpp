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

#include <mtp/ptp/Session.h>
#include <mtp/ptp/Messages.h>
#include <mtp/ptp/Container.h>
#include <mtp/ptp/OperationRequest.h>
#include <mtp/ptp/ByteArrayObjectStream.h>
#include <mtp/ptp/JoinedObjectStream.h>
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
		_packeter(pipe), _sessionId(sessionId), _nextTransactionId(1), _transaction(), _defaultTimeout(10000)
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

	msg::DeviceInfo Session::GetDeviceInfoImpl()
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetDeviceInfo, transaction.Id));
		ByteArray data = Get(transaction.Id);
		InputStream stream(data); //operation code + session id
		msg::DeviceInfo gdi;
		gdi.Read(stream);
		return gdi;
	}


	msg::ObjectHandles Session::GetObjectHandles(StorageId storageId, ObjectFormat objectFormat, ObjectId parent, int timeout)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetObjectHandles, transaction.Id, storageId.Id, static_cast<u32>(objectFormat), parent.Id), timeout);
		ByteArray data = Get(transaction.Id, timeout);
		InputStream stream(data);

		msg::ObjectHandles goh;
		goh.Read(stream);
		return goh;
	}

	msg::StorageIDs Session::GetStorageIDs()
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetStorageIDs, transaction.Id));
		ByteArray data = Get(transaction.Id);
		InputStream stream(data);

		msg::StorageIDs gsi;
		gsi.Read(stream);
		return gsi;
	}

	msg::StorageInfo Session::GetStorageInfo(StorageId storageId)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetStorageInfo, transaction.Id, storageId.Id));
		ByteArray data = Get(transaction.Id);
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
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetObjectInfo, transaction.Id, objectId.Id));
		ByteArray data = Get(transaction.Id);
		InputStream stream(data);
		msg::ObjectInfo goi;
		goi.Read(stream);
		return goi;
	}

	msg::ObjectPropsSupported Session::GetObjectPropsSupported(ObjectId objectId)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetObjectPropsSupported, transaction.Id, objectId.Id));
		ByteArray data = Get(transaction.Id);
		InputStream stream(data);
		msg::ObjectPropsSupported ops;
		ops.Read(stream);
		return ops;
	}

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

	ByteArray Session::GetPartialObject(ObjectId objectId, u64 offset, u32 size)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		if (_getPartialObject64Supported)
			Send(OperationRequest(OperationCode::GetPartialObject64, transaction.Id, objectId.Id, offset, offset >> 32, size));
		else
		{
			if (offset + size > std::numeric_limits<u32>::max())
				throw std::runtime_error("32 bit overflow for GetPartialObject");
			Send(OperationRequest(OperationCode::GetPartialObject, transaction.Id, objectId.Id, offset, size));
		}
		return Get(transaction.Id);
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
	{
		try
		{ EndEditObject(objectId); }
		catch(const std::exception &ex)
		{ }
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::BeginEditObject, transaction.Id, objectId.Id));
		Get(transaction.Id);
	}

	void Session::SendPartialObject(ObjectId objectId, u64 offset, const ByteArray &data)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::SendPartialObject, transaction.Id, objectId.Id, offset, offset >> 32, data.size()));
		{
			DataRequest req(OperationCode::SendPartialObject, transaction.Id);
			IObjectInputStreamPtr inputStream = std::make_shared<ByteArrayObjectInputStream>(data);
			Container container(req, inputStream);
			_packeter.Write(std::make_shared<JoinedObjectInputStream>(std::make_shared<ByteArrayObjectInputStream>(container.Data), inputStream), _defaultTimeout);
		}
		Get(transaction.Id);
	}

	void Session::TruncateObject(ObjectId objectId, u64 size)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		//64 bit size?
		Send(OperationRequest(OperationCode::TruncateObject, transaction.Id, objectId.Id, size, size >> 32));
		Get(transaction.Id);
	}

	void Session::EndEditObject(ObjectId objectId)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::EndEditObject, transaction.Id, objectId.Id));
		Get(transaction.Id);
	}

	void Session::SetObjectProperty(ObjectId objectId, ObjectProperty property, const ByteArray &value)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::SetObjectPropValue, transaction.Id, objectId.Id, (u16)property));
		{
			DataRequest req(OperationCode::SetObjectPropValue, transaction.Id);
			req.Append(value);
			Container container(req);
			_packeter.Write(container.Data, _defaultTimeout);
		}
		Get(transaction.Id);
	}

	StorageId Session::GetObjectStorage(mtp::ObjectId id)
	{ return StorageId(GetObjectIntegerProperty(id, ObjectProperty::StorageId)); }

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
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetObjectPropValue, transaction.Id, objectId.Id, (u16)property));
		return Get(transaction.Id);
	}

	u64 Session::GetObjectIntegerProperty(ObjectId objectId, ObjectProperty property)
	{
		ByteArray data = GetObjectProperty(objectId, property);
		InputStream s(data);
		switch(data.size())
		{
		case 8: return s.Read64();
		case 4: return s.Read32();
		case 2: return s.Read16();
		case 1: return s.Read8();
		default:
			throw std::runtime_error("unexpected length for numeric property");
		}
	}

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
		ByteArray data = GetObjectProperty(objectId, property);
		InputStream s(data);
		std::string value;
		s >> value;
		return value;
	}

	ByteArray Session::GetObjectPropertyList(ObjectId objectId, ObjectFormat format, ObjectProperty property, u32 groupCode, u32 depth)
	{
		if (objectId == Root) //ffffffff -> 0
			objectId = Device;
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetObjectPropList, transaction.Id, objectId.Id, (u32)format, (u32)property, groupCode, depth));
		return Get(transaction.Id);
	}

	void Session::DeleteObject(ObjectId objectId)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::DeleteObject, transaction.Id, objectId.Id, 0));
		Get(transaction.Id);
	}

	ByteArray Session::GetDeviceProperty(DeviceProperty property)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetDevicePropValue, transaction.Id, (u16)property));
		return Get(transaction.Id);
	}

	Session::ObjectEditSession::ObjectEditSession(const SessionPtr & session, ObjectId objectId): _session(session), _objectId(objectId)
	{
		session->BeginEditObject(objectId);
	}

	Session::ObjectEditSession::~ObjectEditSession()
	{
		_session->EndEditObject(_objectId);
	}

	void Session::ObjectEditSession::Truncate(u64 size)
	{
		_session->TruncateObject(_objectId, size);
	}

	void Session::ObjectEditSession::Send(u64 offset, const ByteArray &data)
	{
		_session->SendPartialObject(_objectId, offset, data);
	}

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

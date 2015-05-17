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
#include <mtp/ptp/Session.h>
#include <mtp/ptp/Messages.h>
#include <mtp/ptp/Container.h>
#include <mtp/ptp/OperationRequest.h>
#include <mtp/ptp/ByteArrayObjectStream.h>
#include <mtp/ptp/JoinedObjectStream.h>
#include <limits>

namespace mtp
{

#define CHECK_RESPONSE(RCODE) do { \
	if ((RCODE) != ResponseType::OK && (RCODE) != ResponseType::SessionAlreadyOpen) \
		throw InvalidResponseException(__func__, (RCODE)); \
} while(false)

	Session::Session(usb::BulkPipePtr pipe, u32 sessionId):
		_packeter(pipe), _sessionId(sessionId), _nextTransactionId(1)
	{
		_deviceInfo = GetDeviceInfoImpl();
		_getPartialObject64Supported = _deviceInfo.Supports(OperationCode::GetPartialObject64);
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


	void Session::Send(const OperationRequest &req)
	{
		Container container(req);
		_packeter.Write(container.Data);
	}

	void Session::Close()
	{
		scoped_mutex_lock l(_mutex);
		Send(OperationRequest(OperationCode::CloseSession, 0, _sessionId));
		ByteArray data, response;
		ResponseType responseCode;
		_packeter.Read(0, data, responseCode, response);
		//HexDump("payload", data);
	}

	ByteArray Session::Get(u32 transaction)
	{
		ByteArray data, response;
		ResponseType responseCode;
		_packeter.Read(transaction, data, responseCode, response);
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


	msg::ObjectHandles Session::GetObjectHandles(u32 storageId, u32 objectFormat, u32 parent)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetObjectHandles, transaction.Id, storageId, objectFormat, parent));
		ByteArray data = Get(transaction.Id);
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

	msg::StorageInfo Session::GetStorageInfo(u32 storageId)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetStorageInfo, transaction.Id, storageId));
		ByteArray data = Get(transaction.Id);
		InputStream stream(data);
		msg::StorageInfo gsi;
		gsi.Read(stream);
		return gsi;
	}

	Session::NewObjectInfo Session::CreateDirectory(const std::string &name, mtp::u32 parentId, mtp::u32 storageId, AssociationType type)
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

	msg::ObjectInfo Session::GetObjectInfo(u32 objectId)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetObjectInfo, transaction.Id, objectId));
		ByteArray data = Get(transaction.Id);
		InputStream stream(data);
		msg::ObjectInfo goi;
		goi.Read(stream);
		return goi;
	}

	msg::ObjectPropsSupported Session::GetObjectPropsSupported(u32 objectId)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetObjectPropsSupported, transaction.Id, objectId));
		ByteArray data = Get(transaction.Id);
		InputStream stream(data);
		msg::ObjectPropsSupported ops;
		ops.Read(stream);
		return ops;
	}

	void Session::GetObject(u32 objectId, const IObjectOutputStreamPtr &outputStream)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetObject, transaction.Id, objectId));
		ByteArray response;
		ResponseType responseCode;
		_packeter.Read(transaction.Id, outputStream, responseCode, response);
		CHECK_RESPONSE(responseCode);
	}

	ByteArray Session::GetPartialObject(u32 objectId, u64 offset, u32 size)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		if (_getPartialObject64Supported)
			Send(OperationRequest(OperationCode::GetPartialObject64, transaction.Id, objectId, offset, offset >> 32, size));
		else
		{
			if (offset + size > std::numeric_limits<u32>::max())
				throw std::runtime_error("32 bit overflow for GetPartialObject");
			Send(OperationRequest(OperationCode::GetPartialObject, transaction.Id, objectId, offset, size));
		}
		return Get(transaction.Id);
	}


	Session::NewObjectInfo Session::SendObjectInfo(const msg::ObjectInfo &objectInfo, u32 storageId, u32 parentObject)
	{
		if (objectInfo.Filename.empty())
			throw std::runtime_error("object filename must not be empty");
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::SendObjectInfo, transaction.Id, storageId, parentObject));
		{
			DataRequest req(OperationCode::SendObjectInfo, transaction.Id);
			OutputStream stream(req.Data);
			objectInfo.Write(stream);
			Container container(req);
			_packeter.Write(container.Data);
		}
		ByteArray data, response;
		ResponseType responseCode;
		_packeter.Read(transaction.Id, data, responseCode, response);
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

	void Session::BeginEditObject(u32 objectId)
	{
		try
		{ EndEditObject(objectId); }
		catch(const std::exception &ex)
		{ }
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::BeginEditObject, transaction.Id, objectId));
		Get(transaction.Id);
	}

	void Session::SendPartialObject(u32 objectId, u64 offset, const ByteArray &data)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::SendPartialObject, transaction.Id, objectId, offset, offset >> 32, data.size()));
		{
			DataRequest req(OperationCode::SendPartialObject, transaction.Id);
			IObjectInputStreamPtr inputStream = std::make_shared<ByteArrayObjectInputStream>(data);
			Container container(req, inputStream);
			_packeter.Write(std::make_shared<JoinedObjectInputStream>(std::make_shared<ByteArrayObjectInputStream>(container.Data), inputStream));
		}
		Get(transaction.Id);
	}

	void Session::TruncateObject(u32 objectId, u64 size)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		//64 bit size?
		Send(OperationRequest(OperationCode::TruncateObject, transaction.Id, objectId, size, size >> 32));
		Get(transaction.Id);
	}

	void Session::EndEditObject(u32 objectId)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::EndEditObject, transaction.Id, objectId));
		Get(transaction.Id);
	}

	void Session::SetObjectProperty(u32 objectId, ObjectProperty property, const ByteArray &value)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::SetObjectPropValue, transaction.Id, objectId, (u16)property));
		{
			DataRequest req(OperationCode::SetObjectPropValue, transaction.Id);
			req.Append(value);
			Container container(req);
			_packeter.Write(container.Data);
		}
		Get(transaction.Id);
	}

	void Session::SetObjectProperty(u32 objectId, ObjectProperty property, const std::string &value)
	{
		ByteArray data;
		data.reserve(value.size() * 2 + 1);
		OutputStream stream(data);
		stream << value;
		SetObjectProperty(objectId, property, data);
	}

	ByteArray Session::GetObjectProperty(u32 objectId, ObjectProperty property)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetObjectPropValue, transaction.Id, objectId, (u16)property));
		return Get(transaction.Id);
	}

	u64 Session::GetObjectIntegerProperty(u32 objectId, ObjectProperty property)
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

	std::string Session::GetObjectStringProperty(u32 objectId, ObjectProperty property)
	{
		ByteArray data = GetObjectProperty(objectId, property);
		InputStream s(data);
		std::string value;
		s >> value;
		return value;
	}

	void Session::DeleteObject(u32 objectId)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::DeleteObject, transaction.Id, objectId, 0));
		Get(transaction.Id);
	}

	ByteArray Session::GetDeviceProperty(DeviceProperty property)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::GetDevicePropValue, transaction.Id, (u16)property));
		return Get(transaction.Id);
	}

	Session::ObjectEditSession::ObjectEditSession(const SessionPtr & session, u32 objectId): _session(session), _objectId(objectId)
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

}

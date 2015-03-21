#include <mtp/ptp/Session.h>
#include <mtp/ptp/Messages.h>
#include <mtp/ptp/Container.h>
#include <mtp/ptp/OperationRequest.h>

namespace mtp
{

#define CHECK_RESPONSE(RDATA) do { \
	InputStream s(response); \
	Response header(s);\
	if (header.ResponseType != ResponseType::OK && header.ResponseType != ResponseType::SessionAlreadyOpen) \
		throw InvalidResponseException(__func__, header.ResponseType); \
} while(false)

	void Session::Close()
	{
		OperationRequest req(OperationCode::CloseSession, 0, _sessionId);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data, response;
		_packeter.Read(0, data, response);
		//HexDump("payload", data);
	}

	msg::ObjectHandles Session::GetObjectHandles(u32 storageId, u32 objectFormat, u32 parent)
	{
		u32 transaction = _transactionId++;
		OperationRequest req(OperationCode::GetObjectHandles, transaction, storageId, objectFormat, parent);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data, response;
		_packeter.Read(transaction, data, response);
		CHECK_RESPONSE(response);
		InputStream stream(data, 8); //operation code + session id

		msg::ObjectHandles goh;
		goh.Read(stream);
		return goh;
	}

	msg::StorageIDs Session::GetStorageIDs()
	{
		u32 transaction = _transactionId++;
		OperationRequest req(OperationCode::GetStorageIDs, transaction, 0xffffffffu);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data, response;
		_packeter.Read(transaction, data, response);
		CHECK_RESPONSE(response);
		InputStream stream(data, 8); //operation code + session id

		msg::StorageIDs gsi;
		gsi.Read(stream);
		return gsi;
	}

	msg::StorageInfo Session::GetStorageInfo(u32 storageId)
	{
		u32 transaction = _transactionId++;
		OperationRequest req(OperationCode::GetStorageInfo, transaction, storageId);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data, response;
		_packeter.Read(transaction, data, response);
		CHECK_RESPONSE(response);
		InputStream stream(data, 8); //operation code + session id
		msg::StorageInfo gsi;
		gsi.Read(stream);
		return gsi;
	}

	msg::ObjectInfo Session::GetObjectInfo(u32 objectId)
	{
		u32 transaction = _transactionId++;
		OperationRequest req(OperationCode::GetObjectInfo, transaction, objectId);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data, response;
		_packeter.Read(transaction, data, response);
		CHECK_RESPONSE(response);
		InputStream stream(data, 8); //operation code + session id
		msg::ObjectInfo goi;
		goi.Read(stream);
		return goi;
	}

	msg::ObjectPropsSupported Session::GetObjectPropsSupported(u32 objectId)
	{
		u32 transaction = _transactionId++;
		OperationRequest req(OperationCode::GetObjectPropsSupported, transaction, objectId);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data, response;
		_packeter.Read(transaction, data, response);
		CHECK_RESPONSE(response);
		InputStream stream(data, 8); //operation code + session id
		msg::ObjectPropsSupported ops;
		ops.Read(stream);
		return ops;
	}

	ByteArray Session::GetObject(u32 objectId)
	{
		u32 transaction = _transactionId++;
		OperationRequest req(OperationCode::GetObject, transaction, objectId);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data, response;
		_packeter.Read(transaction, data, response);
		CHECK_RESPONSE(response);
		return ByteArray(data.begin() + 8, data.end());
	}

	Session::NewObjectInfo Session::SendObjectInfo(const msg::ObjectInfo &objectInfo, u32 storageId, u32 parentObject)
	{
		u32 transaction = _transactionId++;
		{
			OperationRequest req(OperationCode::SendObjectInfo, transaction, storageId, parentObject);
			Container container(req);
			_packeter.Write(container.Data);
		}
		{
			DataRequest req(OperationCode::SendObjectInfo, transaction);
			OutputStream stream(req.Data);
			objectInfo.Write(stream);
			Container container(req);
			_packeter.Write(container.Data);
		}
		ByteArray data, response;
		_packeter.Read(transaction, data, response);
		//HexDump("response", response);
		CHECK_RESPONSE(response);
		InputStream stream(response, 8); //operation code + session id
		NewObjectInfo noi;
		stream >> noi.StorageId;
		stream >> noi.ParentObjectId;
		stream >> noi.ObjectId;
		return noi;
	}

	void Session::SendObject(const ByteArray &object)
	{
		u32 transaction = _transactionId++;
		{
			OperationRequest req(OperationCode::SendObject, transaction);
			Container container(req);
			_packeter.Write(container.Data);
		}
		{
			DataRequest req(OperationCode::SendObject, transaction);
			req.Append(object);
			Container container(req);
			_packeter.Write(container.Data, 0);
		}
		ByteArray data, response;
		_packeter.Read(transaction, data, response);
		CHECK_RESPONSE(response);
	}

	void Session::SetObjectProperty(u32 objectId, ObjectProperty property, const ByteArray &value)
	{
		u32 transaction = _transactionId++;
		{
			OperationRequest req(OperationCode::SetObjectPropValue, transaction, objectId, (u16)property);
			Container container(req);
			_packeter.Write(container.Data);
		}
		{
			DataRequest req(OperationCode::SetObjectPropValue, transaction);
			req.Append(value);
			Container container(req);
			_packeter.Write(container.Data, 0);
		}
		ByteArray data, response;
		_packeter.Read(transaction, data, response);
		CHECK_RESPONSE(response);
	}

	ByteArray Session::GetObjectProperty(u32 objectId, ObjectProperty property)
	{
		u32 transaction = _transactionId++;
		{
			OperationRequest req(OperationCode::GetObjectPropValue, transaction, objectId, (u16)property);
			Container container(req);
			_packeter.Write(container.Data);
		}
		ByteArray data, response;
		_packeter.Read(transaction, data, response);
		CHECK_RESPONSE(response);
		return data;
	}

	void Session::SetObjectProperty(u32 objectId, ObjectProperty property, const std::string &value)
	{
		ByteArray data;
		data.reserve(value.size() * 2 + 1);
		OutputStream stream(data);
		stream << value;
		SetObjectProperty(objectId, property, data);
	}


	void Session::DeleteObject(u32 objectId)
	{
		u32 transaction = _transactionId++;
		OperationRequest req(OperationCode::DeleteObject, transaction, objectId);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data, response;
		_packeter.Read(transaction, data, response);
		CHECK_RESPONSE(response);
	}

}

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

	Session::Session(const PipePacketer & packeter, u32 sessionId):
		_packeter(packeter), _sessionId(sessionId), _nextTransactionId(1), _transaction(),
		_getObjectModificationTimeBuggy(false), _separateBulkWrites(false),
		_defaultTimeout(DefaultTimeout)
	{
		_deviceInfo = GetDeviceInfo(_packeter, _defaultTimeout);
		if (_deviceInfo.Manufacturer == "Microsoft")
			_separateBulkWrites = true;

		_getPartialObject64Supported = _deviceInfo.Supports(OperationCode::GetPartialObject64);
		_getObjectPropertyListSupported = _deviceInfo.Supports(OperationCode::GetObjectPropList);
		_getObjectPropValueSupported = _deviceInfo.Supports(OperationCode::GetObjectPropValue);
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

	void Session::Send(PipePacketer &packeter, const OperationRequest &req, int timeout)
	{
		if (timeout <= 0)
			timeout = DefaultTimeout;
		Container container(req);
		packeter.Write(container.Data, timeout);
	}


	void Session::Send(const OperationRequest &req, int timeout)
	{
		if (timeout <= 0)
			timeout = _defaultTimeout;
		Send(_packeter, req, timeout);
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

	ByteArray Session::Get(PipePacketer &packeter, u32 transaction, ByteArray & response, int timeout)
	{
		if (timeout <= 0)
			timeout = DefaultTimeout;

		ByteArray data;
		ResponseType responseCode;
		packeter.Read(transaction, data, responseCode, response, timeout);
		CHECK_RESPONSE(responseCode);
		return data;
	}

	ByteArray Session::Get(u32 transaction, ByteArray &response, int timeout)
	{
		if (timeout <= 0)
			timeout = _defaultTimeout;
		return Get(_packeter, transaction, response, timeout);
	}

	template<typename ... Args>
	ByteArray Session::RunTransactionWithDataRequest(int timeout, OperationCode code, ByteArray & response, const IObjectInputStreamPtr & inputStream, Args && ... args)
	{
#if 0
		try
		{ _packeter.PollEvent(_defaultTimeout); }
		catch(const std::exception &ex)
		{ error("exception in interrupt: ", ex.what()); }
#endif

		scoped_mutex_lock l(_mutex);
		if (!_deviceInfo.Supports(code))
			throw std::runtime_error("Operation code " + ToString(code) + " not supported.");
		Transaction transaction(this);
		Send(OperationRequest(code, transaction.Id, std::forward<Args>(args) ... ), timeout);
		if (inputStream)
		{
			DataRequest req(code, transaction.Id);
			Container container(req, inputStream);
			if (_separateBulkWrites)
			{
				_packeter.Write(std::make_shared<ByteArrayObjectInputStream>(container.Data), timeout);
				_packeter.Write(inputStream, timeout);
			} else
				_packeter.Write(std::make_shared<JoinedObjectInputStream>(std::make_shared<ByteArrayObjectInputStream>(container.Data), inputStream), timeout);
		}
		return Get(transaction.Id, response);
	}

	msg::DeviceInfo Session::GetDeviceInfo(PipePacketer& packeter, int timeout)
	{
		ByteArray response;
		Send(packeter, OperationRequest(OperationCode::GetDeviceInfo, 0), timeout);
		auto info = Get(packeter, 0, response, timeout);
		return ParseResponse<msg::DeviceInfo>(info);
	}


	msg::ObjectHandles Session::GetObjectHandles(StorageId storageId, ObjectFormat objectFormat, ObjectId parent, int timeout)
	{ return ParseResponse<msg::ObjectHandles>(RunTransaction(timeout, OperationCode::GetObjectHandles, storageId.Id, static_cast<u32>(objectFormat), parent.Id)); }

	msg::StorageIDs Session::GetStorageIDs()
	{ return ParseResponse<msg::StorageIDs>(RunTransaction(_defaultTimeout, OperationCode::GetStorageIDs)); }

	msg::StorageInfo Session::GetStorageInfo(StorageId storageId)
	{ return ParseResponse<msg::StorageInfo>(RunTransaction(_defaultTimeout, OperationCode::GetStorageInfo, storageId.Id)); }

	msg::SendObjectPropListResponse Session::SendObjectPropList(StorageId storageId, ObjectId parentId, ObjectFormat format, u64 objectSize, const ByteArray & propList)
	{
		ByteArray responseData;
		IObjectInputStreamPtr inputStream = std::make_shared<ByteArrayObjectInputStream>(propList);
		RunTransactionWithDataRequest(_defaultTimeout, OperationCode::SendObjectPropList, responseData, inputStream, storageId.Id, parentId.Id, static_cast<u32>(format), static_cast<u32>(objectSize >> 32), static_cast<u32>(objectSize));
		return ParseResponse<msg::SendObjectPropListResponse>(responseData);
	}

	msg::NewObjectInfo Session::CreateDirectory(const std::string &name, ObjectId parentId, StorageId storageId, AssociationType type)
	{
		if (_deviceInfo.Supports(OperationCode::SendObjectPropList))
		{
			//modern way of creating objects
			ByteArray propList;
			{
				OutputStream os(propList);
				os.Write32(1); //number of props
				os.Write32(0); //object handle
				os.Write16(static_cast<u16>(ObjectProperty::ObjectFilename));
				os.Write16(static_cast<u16>(DataTypeCode::String));
				os.WriteString(name);
			}
			auto response = SendObjectPropList(storageId, parentId, ObjectFormat::Association, 0, propList);

			msg::NewObjectInfo noi;
			noi.StorageId = response.StorageId;
			noi.ParentObjectId = response.ParentObjectId;
			noi.ObjectId = response.ObjectId;
			return noi;
		}
		else
		{
			mtp::msg::ObjectInfo oi;
			oi.Filename = name;
			oi.ParentObject = parentId;
			oi.StorageId = storageId;
			oi.ObjectFormat = mtp::ObjectFormat::Association;
			oi.AssociationType = type;
			return SendObjectInfo(oi, storageId, parentId);
		}
	}

	msg::ObjectInfo Session::GetObjectInfo(ObjectId objectId)
	{ return ParseResponse<msg::ObjectInfo>(RunTransaction(_defaultTimeout, OperationCode::GetObjectInfo, objectId.Id)); }

	msg::ObjectPropertiesSupported Session::GetObjectPropertiesSupported(ObjectFormat format)
	{ return ParseResponse<msg::ObjectPropertiesSupported>(RunTransaction(_defaultTimeout, OperationCode::GetObjectPropsSupported, static_cast<u32>(format))); }

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


	msg::NewObjectInfo Session::SendObjectInfo(const msg::ObjectInfo &objectInfo, StorageId storageId, ObjectId parentObject)
	{
		if (objectInfo.Filename.empty())
			throw std::runtime_error("object filename must not be empty");

		if (_deviceInfo.Supports(OperationCode::SendObjectPropList))
		{
			//modern way of creating objects
			ByteArray propList;
			{
				OutputStream os(propList);
				os.Write32(1); //number of props
				os.Write32(0); //object handle
				os.Write16(static_cast<u16>(ObjectProperty::ObjectFilename));
				os.Write16(static_cast<u16>(DataTypeCode::String));
				os.WriteString(objectInfo.Filename);
			}

			auto response = SendObjectPropList(storageId, parentObject, objectInfo.ObjectFormat, objectInfo.ObjectCompressedSize, propList);

			msg::NewObjectInfo noi;
			noi.StorageId = response.StorageId;
			noi.ParentObjectId = response.ParentObjectId;
			noi.ObjectId = response.ObjectId;
			return noi;
		}

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
		return ParseResponse<msg::NewObjectInfo>(response);
	}

	void Session::SendObject(const IObjectInputStreamPtr &inputStream, int timeout)
	{
		scoped_mutex_lock l(_mutex);
		Transaction transaction(this);
		Send(OperationRequest(OperationCode::SendObject, transaction.Id));
		{
			DataRequest req(OperationCode::SendObject, transaction.Id);
			Container container(req, inputStream);
			_packeter.Write(std::make_shared<ByteArrayObjectInputStream>(container.Data), timeout);
			_packeter.Write(inputStream, timeout);
		}
		ByteArray response;
		Get(transaction.Id, response);
	}

	void Session::BeginEditObject(ObjectId objectId)
	{ RunTransaction(_defaultTimeout, OperationCode::BeginEditObject, objectId.Id); }

	void Session::SendPartialObject(ObjectId objectId, u64 offset, const ByteArray &data)
	{
		IObjectInputStreamPtr inputStream = std::make_shared<ByteArrayObjectInputStream>(data);
		ByteArray response;
		RunTransactionWithDataRequest(_defaultTimeout, OperationCode::SendPartialObject, response, inputStream, objectId.Id, offset, offset >> 32, data.size());
	}

	void Session::TruncateObject(ObjectId objectId, u64 size)
	{ RunTransaction(_defaultTimeout, OperationCode::TruncateObject, objectId.Id, size, size >> 32); }

	void Session::EndEditObject(ObjectId objectId)
	{ RunTransaction(_defaultTimeout, OperationCode::EndEditObject, objectId.Id); }

	void Session::SetObjectProperty(ObjectId objectId, ObjectProperty property, const ByteArray &value)
	{
		IObjectInputStreamPtr inputStream = std::make_shared<ByteArrayObjectInputStream>(value);
		ByteArray response;
		RunTransactionWithDataRequest(_defaultTimeout, OperationCode::SetObjectPropValue, response, inputStream, objectId.Id, (u16)property);
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
	{
		if (!_getObjectPropValueSupported) {
			auto info = GetObjectInfo(objectId);
			switch(property)
			{
				case ObjectProperty::StorageId:
					return info.StorageId.Id;
				case ObjectProperty::ObjectFormat:
					return static_cast<u32>(info.ObjectFormat);
				case ObjectProperty::ProtectionStatus:
					return info.ProtectionStatus;
				case ObjectProperty::ObjectSize:
					return info.ObjectCompressedSize;
				case ObjectProperty::RepresentativeSampleFormat:
					return info.ThumbFormat;
				case ObjectProperty::RepresentativeSampleSize:
					return info.ThumbCompressedSize;
				case ObjectProperty::RepresentativeSampleWidth:
					return info.ThumbPixWidth;
				case ObjectProperty::RepresentativeSampleHeight:
					return info.ThumbPixHeight;
				case ObjectProperty::Width:
					return info.ImagePixWidth;
				case ObjectProperty::Height:
					return info.ImagePixHeight;
				case ObjectProperty::ImageBitDepth:
					return info.ImageBitDepth;
				case ObjectProperty::ParentObject:
					return info.ParentObject.Id;
				case ObjectProperty::AssociationType:
					return static_cast<u32>(info.AssociationType);
				case ObjectProperty::AssociationDesc:
					return info.AssociationDesc;
				default:
					throw std::runtime_error("Device does not support object properties and no ObjectInfo fallback for " + ToString(property) + ".");
			}
		}
		return ReadSingleInteger(GetObjectProperty(objectId, property));
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

	void Session::SetObjectPropertyAsArray(ObjectId objectId, ObjectProperty property, const ByteArray &value)
	{
		auto n = value.size();

		ByteArray array;
		OutputStream out(array);
		array.reserve(n + 4);
		out.WriteArray(value);

		SetObjectProperty(objectId, property, array);
	}

	std::string Session::GetObjectStringProperty(ObjectId objectId, ObjectProperty property)
	{
		if (!_getObjectPropValueSupported) {
			auto info = GetObjectInfo(objectId);
			switch(property)
			{
				case ObjectProperty::ObjectFilename:
					return info.Filename;
				case ObjectProperty::DateCreated:
				case ObjectProperty::DateAuthored:
				case ObjectProperty::DateAdded:
					return info.CaptureDate;
				case ObjectProperty::DateModified:
					return info.ModificationDate;
				default:
					throw std::runtime_error("Device does not support object properties and no ObjectInfo fallback for " + ToString(property) + ".");
			}
		}
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
		ByteArray response;
		RunTransactionWithDataRequest(_defaultTimeout, OperationCode::SetDevicePropValue, response, inputStream, (u16)property);
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
	{ return RunTransaction(timeout, OperationCode::GetObjectPropList, objectId.Id, (u32)format, property != ObjectProperty::All? (u32)property: 0xffffffffu, groupCode, depth); }

	void Session::DeleteObject(ObjectId objectId, int timeout)
	{ RunTransaction(timeout, OperationCode::DeleteObject, objectId.Id, 0); }

	msg::DevicePropertyDesc Session::GetDevicePropertyDesc(DeviceProperty property)
	{ return ParseResponse<msg::DevicePropertyDesc>(RunTransaction(_defaultTimeout, OperationCode::GetDevicePropDesc, static_cast<u32>(property))); }

	ByteArray Session::GetDeviceProperty(DeviceProperty property)
	{ return RunTransaction(_defaultTimeout, OperationCode::GetDevicePropValue, static_cast<u32>(property)); }

	ByteArray Session::GenericOperation(OperationCode code)
	{ return RunTransaction(_defaultTimeout, code); }

	ByteArray Session::GenericOperation(OperationCode code, const ByteArray & payload)
	{
		IObjectInputStreamPtr inputStream = std::make_shared<ByteArrayObjectInputStream>(payload);
		ByteArray response;
		return RunTransactionWithDataRequest(_defaultTimeout, code, response, inputStream);
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

	void Session::SetObjectReferences(ObjectId objectId, const msg::ObjectHandles & objects)
	{
		ByteArray data;
		OutputStream out(data);
		objects.Write(out);

		IObjectInputStreamPtr inputStream = std::make_shared<ByteArrayObjectInputStream>(data);
		ByteArray response;
		RunTransactionWithDataRequest(_defaultTimeout, OperationCode::SetObjectReferences, response, inputStream, objectId.Id);
	}

	msg::ObjectHandles Session::GetObjectReferences(ObjectId objectId)
	{ return ParseResponse<msg::ObjectHandles>(RunTransaction(_defaultTimeout, OperationCode::GetObjectReferences, objectId.Id)); }

}

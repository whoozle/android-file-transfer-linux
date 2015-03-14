#include <mtp/ptp/Session.h>
#include <mtp/ptp/Messages.h>
#include <mtp/ptp/Container.h>
#include <mtp/ptp/OperationRequest.h>

namespace mtp
{

	void Session::Close()
	{
		OperationRequest req(OperationCode::CloseSession, 0, _sessionId);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data = _packeter.Read();
		//HexDump("payload", data);
	}

	msg::ObjectHandles Session::GetObjectHandles(u32 storageId, u32 objectFormat, u32 parent)
	{
		OperationRequest req(OperationCode::GetObjectHandles, _transactionId++, storageId, objectFormat, parent);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data = _packeter.Read();
		Stream stream(data, 8); //operation code + session id

		msg::ObjectHandles goh;
		goh.Read(stream);
		return std::move(goh);
	}
	msg::StorageIDs Session::GetStorageIDs()
	{
		OperationRequest req(OperationCode::GetStorageIDs, _transactionId++, 0xffffffffu);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data = _packeter.Read();
		Stream stream(data, 8); //operation code + session id

		msg::StorageIDs gsi;
		gsi.Read(stream);
		return std::move(gsi);
	}

	msg::StorageInfo Session::GetStorageInfo(u32 storageId)
	{
		OperationRequest req(OperationCode::GetStorageInfo, _transactionId++, storageId);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data = _packeter.Read();
		Stream stream(data, 8); //operation code + session id
		msg::StorageInfo gsi;
		gsi.Read(stream);
		return std::move(gsi);
	}

	msg::ObjectInfo Session::GetObjectInfo(u32 objectId)
	{
		OperationRequest req(OperationCode::GetObjectInfo, _transactionId++, objectId);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data = _packeter.Read();
		Stream stream(data, 8); //operation code + session id
		msg::ObjectInfo goi;
		goi.Read(stream);
		return std::move(goi);
	}

	void Session::DeleteObject(u32 objectId)
	{
		OperationRequest req(OperationCode::DeleteObject, _transactionId++, objectId);
		Container container(req);
		_packeter.Write(container.Data);
		_packeter.Read();
	}

}

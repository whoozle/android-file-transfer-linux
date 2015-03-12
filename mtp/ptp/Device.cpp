#include <mtp/ptp/Device.h>
#include <mtp/ptp/Response.h>
#include <mtp/ptp/Container.h>
#include <mtp/ptp/OperationRequest.h>


namespace mtp
{
	msg::ObjectHandles Session::GetObjectHandles(u32 storageId, u32 objectFormat)
	{
		OperationRequest req(OperationCode::GetObjectHandles, _sessionId, storageId, objectFormat);
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
		OperationRequest req(OperationCode::GetStorageIDs, _sessionId, 0xffffffffu);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data = _packeter.Read();
		Stream stream(data, 8); //operation code + session id

		msg::StorageIDs gsi;
		gsi.Read(stream);
		return std::move(gsi);
	}

	msg::StorageInfo Session::GetStorageInfo(u32 storageId, u32 formatCode)
	{
		OperationRequest req(OperationCode::GetStorageInfo, _sessionId, storageId, formatCode);
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
		OperationRequest req(OperationCode::GetObjectInfo, _sessionId, objectId);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data = _packeter.Read();
		Stream stream(data, 8); //operation code + session id
		msg::ObjectInfo goi;
		goi.Read(stream);
		return std::move(goi);
	}

	msg::DeviceInfo Device::GetDeviceInfo()
	{
		OperationRequest req(OperationCode::GetDeviceInfo, 0, 0);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data = _packeter.Read();
		HexDump("payload", data);

		Stream stream(data, 8); //operation code + session id
		msg::DeviceInfo gdi;
		gdi.Read(stream);
		return std::move(gdi);
	}

	SessionPtr Device::OpenSession(u32 sessionId)
	{
		OperationRequest req(OperationCode::OpenSession, 0, 1);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data = _packeter.Read();
		HexDump("payload", data);

		return std::make_shared<Session>(_packeter.GetPipe(), sessionId);
	}

	void PipePacketer::Write(const ByteArray &data)
	{
		HexDump("send", data);
		_pipe->Write(data);
	}

	ByteArray PipePacketer::ReadMessage()
	{
		ByteArray result;
		u32 size = ~0u;
		size_t offset = 0;
		size_t packet_offset;
		while(true)
		{
			ByteArray data = _pipe->Read();
			if (size == ~0u)
			{
				Stream stream(data);
				stream >> size;
				printf("DATA SIZE = %u\n", size);
				if (size < 4)
					throw std::runtime_error("invalid size");
				packet_offset = 4;
				result.resize(size - 4);
			}
			else
				packet_offset = 0;
			HexDump("recv", data);

			size_t src_n = std::min(data.size() - packet_offset, result.size() - offset);
			std::copy(data.begin() + packet_offset, data.begin() + packet_offset + src_n, result.begin() + offset);
			offset += data.size();
			if (offset >= result.size())
				break;
		}
		return result;

	}

	ByteArray PipePacketer::Read()
	{
		_pipe->ReadInterrupt();
		ByteArray message = ReadMessage();
		HexDump("message", message);
		Stream stream(message);
		u16 raw_code;
		stream >> raw_code;
		ContainerType type = ContainerType(raw_code);
		if (type == ContainerType::Response)
			return ByteArray();

		ByteArray response = ReadMessage();
		HexDump("response", response);
		return message;
	}

}

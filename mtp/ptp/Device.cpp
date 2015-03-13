#include <mtp/ptp/Device.h>
#include <mtp/ptp/Response.h>
#include <mtp/ptp/Container.h>
#include <mtp/usb/Context.h>
#include <mtp/ptp/OperationRequest.h>


namespace mtp
{

	void Session::Close()
	{
		OperationRequest req(OperationCode::CloseSession, 0, _sessionId);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data = _packeter.Read();
		HexDump("payload", data);
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

	msg::DeviceInfo Device::GetDeviceInfo()
	{
		OperationRequest req(OperationCode::GetDeviceInfo);
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
		OperationRequest req(OperationCode::OpenSession, 0, sessionId);
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

	DevicePtr Device::Find()
	{
		using namespace mtp;
		usb::ContextPtr ctx(new usb::Context);

		for (usb::DeviceDescriptorPtr desc : ctx->GetDevices())
		{
			usb::DevicePtr device = desc->TryOpen(ctx);
			if (!device)
				continue;
			int confs = desc->GetConfigurationsCount();
			printf("configurations: %d\n", confs);

			for(int i = 0; i < confs; ++i)
			{
				usb::ConfigurationPtr conf = desc->GetConfiguration(i);
				int interfaces = conf->GetInterfaceCount();
				printf("interfaces: %d\n", interfaces);
				for(int j = 0; j < interfaces; ++j)
				{
					usb::InterfacePtr iface = conf->GetInterface(conf, j, 0);
					printf("%d:%d index %u, eps %u\n", i, j, iface->GetIndex(), iface->GetEndpointsCount());
					int name_idx = iface->GetNameIndex();
					if (!name_idx)
						continue;
					std::string name = device->GetString(name_idx);
					if (name == "MTP")
					{
						//device->SetConfiguration(configuration->GetIndex());
						usb::BulkPipePtr pipe = usb::BulkPipe::Create(device, iface);
						return std::make_shared<Device>(pipe);
						break;
					}
				}
			}
		}

		return nullptr;
	}

}

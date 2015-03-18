#include <mtp/ptp/Device.h>
#include <mtp/ptp/Response.h>
#include <mtp/ptp/Container.h>
#include <mtp/ptp/Messages.h>
#include <mtp/usb/Context.h>
#include <mtp/ptp/OperationRequest.h>


namespace mtp
{

	msg::DeviceInfo Device::GetDeviceInfo()
	{
		OperationRequest req(OperationCode::GetDeviceInfo, 0);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data, response;
		_packeter.Read(0, data, response);
		//HexDump("payload", data);

		InputStream stream(data, 8); //operation code + session id
		msg::DeviceInfo gdi;
		gdi.Read(stream);
		return gdi;
	}

	SessionPtr Device::OpenSession(u32 sessionId)
	{
		OperationRequest req(OperationCode::OpenSession, 0, sessionId);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data, response;
		_packeter.Read(0, data, response);
		//HexDump("payload", data);

		return std::make_shared<Session>(_packeter.GetPipe(), sessionId);
	}

	void PipePacketer::Write(const ByteArray &data, int timeout)
	{
		//HexDump("send", data);
		_pipe->Write(data, timeout);
	}

	ByteArray PipePacketer::ReadMessage(int timeout)
	{
		ByteArray result;
		u32 size = ~0u;
		size_t offset = 0;
		size_t packet_offset;
		while(true)
		{
			ByteArray data = _pipe->Read(timeout);
			if (size == ~0u)
			{
				InputStream stream(data);
				stream >> size;
				//printf("DATA SIZE = %u\n", size);
				if (size < 4)
					throw std::runtime_error("invalid size");
				packet_offset = 4;
				result.resize(size - 4);
			}
			else
				packet_offset = 0;
			//HexDump("recv", data);

			size_t src_n = std::min(data.size() - packet_offset, result.size() - offset);
			std::copy(data.begin() + packet_offset, data.begin() + packet_offset + src_n, result.begin() + offset);
			offset += data.size();
			if (offset >= result.size())
				break;
		}
		return result;

	}

	void PipePacketer::Read(u32 transaction, ByteArray &data, ByteArray &response, int timeout)
	{
		_pipe->ReadInterrupt();
		data.clear();
		response.clear();

		ByteArray message;
		u16 raw_code;
		while(true)
		{
			message = ReadMessage(timeout);
			HexDump("message", message);
			InputStream stream(message);
			u16 op_code;
			u32 t;
			stream >> raw_code;
			stream >> op_code;
			stream >> t;
			if (t == transaction)
				break;

			printf("drop message %04x %04x, transaction %08x\n", raw_code, op_code, t);
		}

		ContainerType type = ContainerType(raw_code);
		if (type == ContainerType::Data)
		{
			data = std::move(message);
			response = ReadMessage(timeout);
		}
		else
		{
			response = std::move(message);
		}

		//HexDump("response", response);
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
					}
				}
			}
		}

		return nullptr;
	}

}

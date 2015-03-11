#include <mtp/ptp/Protocol.h>
#include <mtp/ptp/Response.h>
#include <mtp/ptp/Container.h>

namespace mtp
{
	void Protocol::Write(const ByteArray &data)
	{
		HexDump("send", data);
		_pipe->Write(data);
	}

	ByteArray Protocol::ReadMessage()
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

	ByteArray Protocol::Read()
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

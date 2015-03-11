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

	ByteArray Protocol::Read()
	{
		ByteArray result;
		while(true)
		{
			ByteArray data = _pipe->Read();
			HexDump("recv", data);
			ContainerType type;
			ByteArray payload;
			Container::Read(data, type, payload);
			if (type == ContainerType::Response)
				break;
			result = payload;
		}
		return result;
	}
}

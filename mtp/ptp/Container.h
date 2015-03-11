#ifndef CONTAINER_H
#define	CONTAINER_H

#include <mtp/types.h>
#include <mtp/ptp/Packet.h>
#include <mtp/ptp/Stream.h>

namespace mtp
{

	enum struct ContainerType : u16
	{
		Command = 1,
		Data = 2,
		Response = 3,
		Event = 4,
	};

	struct Container : Packet
	{
		template<typename Message>
		Container(const Message &msg)
		{
			Write((u32)msg.Data.size() + 6);
			Write((u16)Message::Type);
			std::copy(msg.Data.begin(), msg.Data.end(), std::back_inserter(Data));
		}

		static void Read(const ByteArray &src, ContainerType &type, ByteArray &data)
		{
			Stream stream(src);
			u32 size;
			u16 raw_type;
			stream >> size;
			stream >> raw_type;
			if (size < 6)
				throw std::runtime_error("invalid size");

			type = ContainerType(raw_type);
			data.assign(src.begin() + 6, src.end());
			if (data.size() + 6 < size)
				throw std::runtime_error("short read");
		}
	};

}

#endif	/* CONTAINER_H */

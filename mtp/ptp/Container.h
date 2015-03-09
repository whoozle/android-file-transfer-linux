#ifndef CONTAINER_H
#define	CONTAINER_H

#include <mtp/types.h>
#include <mtp/ptp/Packet.h>

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
	};
}

#endif	/* CONTAINER_H */

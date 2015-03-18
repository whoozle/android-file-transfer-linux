#ifndef CONTAINER_H
#define	CONTAINER_H

#include <mtp/types.h>
#include <mtp/ptp/InputStream.h>
#include <mtp/ptp/OutputStream.h>
#include <mtp/ptp/Response.h>

namespace mtp
{
	struct Container
	{
		ByteArray Data;

		template<typename Message>
		Container(const Message &msg)
		{
			OutputStream stream(Data);
			stream << ((u32)msg.Data.size() + 6);
			stream << ((u16)Message::Type);
			std::copy(msg.Data.begin(), msg.Data.end(), std::back_inserter(Data));
		}
	};

}

#endif	/* CONTAINER_H */

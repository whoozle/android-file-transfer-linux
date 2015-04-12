/*
 * Android File Transfer for Linux: MTP client for android devices
 * Copyright (C) 2015  Vladimir Menshakov

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef CONTAINER_H
#define	CONTAINER_H

#include <mtp/types.h>
#include <mtp/ptp/OutputStream.h>
#include <mtp/ptp/Response.h>
#include <mtp/ptp/IObjectStream.h>
#include <mtp/ptp/ObjectFormat.h>

namespace mtp
{
	struct Container
	{
		ByteArray Data;

		void WriteSize(OutputStream &stream, u64 size)
		{
			if (size > MaxObjectSize)
				size = MaxObjectSize;
			stream << (u32)size;
		}

		template<typename Message>
		Container(const Message &msg)
		{
			OutputStream stream(Data);
			WriteSize(stream, msg.Data.size() + 6);
			stream << Message::Type;
			std::copy(msg.Data.begin(), msg.Data.end(), std::back_inserter(Data));
		}

		template<typename Message>
		Container(const Message &msg, IObjectInputStreamPtr inputStream)
		{
			OutputStream stream(Data);
			WriteSize(stream, inputStream->GetSize() + msg.Data.size() + 6);
			stream << Message::Type;
			std::copy(msg.Data.begin(), msg.Data.end(), std::back_inserter(Data));
		}
	};

}

#endif	/* CONTAINER_H */

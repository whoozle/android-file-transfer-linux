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

#include <mtp/ptp/PipePacketer.h>
#include <mtp/ptp/Response.h>
#include <mtp/ptp/InputStream.h>


namespace mtp
{

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
				//fprintf(stderr, "DATA SIZE = %u\n", size);
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

	void PipePacketer::PollEvent()
	{
		ByteArray interruptData = _pipe->ReadInterrupt();
		if (interruptData.empty())
			return;

		HexDump("interrupt", interruptData);
		InputStream stream(interruptData);
		ContainerType containerType;
		u32 size;
		u16 eventCode;
		u32 sessionId;
		u32 transactionId;
		stream >> size;
		stream >> containerType;
		stream >> eventCode;
		stream >> sessionId;
		stream >> transactionId;
		if (containerType != ContainerType::Event)
			throw std::runtime_error("not an event");
		fprintf(stderr, "event %04x\n", eventCode);
	}


	void PipePacketer::Read(u32 transaction, ByteArray &data, ByteArray &response, bool ignoreTransaction, int timeout)
	{
		try
		{ PollEvent(); }
		catch(const std::exception &ex)
		{ fprintf(stderr, "exception in interrupt: %s\n", ex.what()); }

		data.clear();
		response.clear();

		ByteArray message;
		Response header;
		while(true)
		{
			message = ReadMessage(timeout);
			//HexDump("message", message);
			InputStream stream(message);
			header.Read(stream);
			if (ignoreTransaction || header.Transaction == transaction)
				break;

			fprintf(stderr, "drop message %04x %04x, transaction %08x, expected: %08x\n", header.ContainerType, header.ResponseType, header.Transaction, transaction);
		}

		if (header.ContainerType == ContainerType::Data)
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

}

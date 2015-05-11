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
#ifndef PIPEPACKETER_H
#define PIPEPACKETER_H

#include <mtp/usb/BulkPipe.h>
#include <mtp/ptp/IObjectStream.h>
#include <mtp/ptp/Response.h>

namespace mtp
{
	class Device;
	DECLARE_PTR(Device);

	class PipePacketer
	{
		usb::BulkPipePtr	_pipe;

	public:
		PipePacketer(const usb::BulkPipePtr &pipe): _pipe(pipe) { }

		usb::BulkPipePtr GetPipe() const
		{ return _pipe; }

		void Write(const IObjectInputStreamPtr &inputStream, int timeout = 10000);
		void Write(const ByteArray &data, int timeout = 10000);

		void Read(u32 transaction, const IObjectOutputStreamPtr &outputStream, ResponseType &code, ByteArray &response, int timeout = 10000);
		void Read(u32 transaction, ByteArray &data, ResponseType &code, ByteArray &response, int timeout = 10000);

		void PollEvent();

	private:
		void ReadMessage(const IObjectOutputStreamPtr &outputStream, int timeout);
	};

}

#endif

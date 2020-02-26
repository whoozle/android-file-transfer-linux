/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2020  Vladimir Menshakov

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef AFTL_MTP_PTP_PIPEPACKETER_H
#define AFTL_MTP_PTP_PIPEPACKETER_H

#include <mtp/usb/BulkPipe.h>
#include <mtp/ptp/IObjectStream.h>
#include <mtp/ptp/Response.h>

namespace mtp
{
	class Device;
	DECLARE_PTR(Device);

	class PipePacketer //! BulkPipe high-level controller class, package all read/write operation into streams and send it to BulkPipe
	{
		usb::BulkPipePtr	_pipe;

	public:
		PipePacketer(const usb::BulkPipePtr &pipe): _pipe(pipe) { }

		usb::BulkPipePtr GetPipe() const
		{ return _pipe; }

		void Write(const IObjectInputStreamPtr &inputStream, int timeout);
		void Write(const ByteArray &data, int timeout);

		void Read(u32 transaction, const IObjectOutputStreamPtr &outputStream, ResponseType &code, ByteArray &response, int timeout);
		void Read(u32 transaction, ByteArray &data, ResponseType &code, ByteArray &response, int timeout);

		void PollEvent(int timeout);
		void Abort(u32 transaction, int timeout);

	private:
		void ReadMessage(const IObjectOutputStreamPtr &outputStream, int timeout);
	};

}

#endif

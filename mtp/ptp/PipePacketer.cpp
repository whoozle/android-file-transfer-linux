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

#include <mtp/ptp/PipePacketer.h>
#include <mtp/ptp/Response.h>
#include <mtp/ptp/EventCode.h>
#include <mtp/ptp/InputStream.h>
#include <mtp/ptp/ByteArrayObjectStream.h>
#include <mtp/ptp/JoinedObjectStream.h>
#include <mtp/ptp/OutputStream.h>
#include <mtp/ptp/OperationRequest.h>
#include <mtp/usb/Request.h>
#include <usb/Device.h>
#include <usb/Interface.h>
#include <mtp/log.h>


namespace mtp
{

	void PipePacketer::Write(const IObjectInputStreamPtr &inputStream, int timeout)
	{ _pipe->Write(inputStream, timeout); }

	void PipePacketer::Write(const ByteArray &data, int timeout)
	{ Write(std::make_shared<ByteArrayObjectInputStream>(data), timeout); }

	namespace
	{
		class MessageParsingStream final: public JoinedObjectOutputStreamBase
		{
			FixedSizeByteArrayObjectOutputStreamPtr	_header;
			IObjectOutputStreamPtr					_stream;
			u64										_offset;
			u64										_size;

		private:
			IObjectOutputStreamPtr GetStream1() const override
			{ return _header; }

			IObjectOutputStreamPtr GetStream2() const override
			{ return _stream; }

		public:
			MessageParsingStream(IObjectOutputStreamPtr stream):
				_header(new FixedSizeByteArrayObjectOutputStream(4)),
				_stream(stream), _offset(0), _size(4)
			{ }

			bool Finished() const
			{ return _offset >= _size; }

			void OnStream1Exhausted() override
			{
				InputStream is(_header->GetData());
				u32 size;
				is >> size;
				if (size < 4)
					throw std::runtime_error("invalid size/malformed message");
				_size = size;
			}

			size_t Write(const u8 *data, size_t size) override
			{
				size_t r = JoinedObjectOutputStreamBase::Write(data, size);
				_offset += r;
				if (!_stream1Exhausted && _offset >= _header->GetData().size()) {
					_stream1Exhausted = true;
					OnStream1Exhausted();
				}
				return r;
			}
		};
		DECLARE_PTR(MessageParsingStream);
	}

	void PipePacketer::ReadMessage(const IObjectOutputStreamPtr &outputStream, int timeout)
	{ _pipe->Read(outputStream, timeout); }

	void PipePacketer::PollEvent(int timeout)
	{
		ByteArray interruptData = _pipe->ReadInterrupt(timeout);
		if (interruptData.empty())
			return;

		HexDump("interrupt", interruptData);
		InputStream stream(interruptData);
		ContainerType containerType;
		u32 size;
		EventCode eventCode;
		u32 sessionId;
		u32 transactionId;
		stream >> size;
		stream >> containerType;
		stream >> eventCode;
		stream >> sessionId;
		stream >> transactionId;
		if (containerType != ContainerType::Event)
			throw std::runtime_error("not an event");
		debug("event ", hex(eventCode, 8));
	}

	namespace
	{
		struct DummyOutputStream final: IObjectOutputStream, public CancellableStream
		{
			size_t Write(const u8 *data, size_t size) override
			{ return size; }
		};

		class HeaderParserObjectOutputStream final: public JoinedObjectOutputStreamBase
		{
			size_t									_offset;
			u32										_transaction;
			FixedSizeByteArrayObjectOutputStreamPtr	_header;
			ByteArrayObjectOutputStreamPtr			_response;
			IObjectOutputStreamPtr					_dataOutput;
			IObjectOutputStreamPtr					_output;
			bool									_valid;
			bool									_finished;
			ResponseType							_responseCode;

		private:
			virtual IObjectOutputStreamPtr GetStream1() const override
			{ return _header; }
			virtual IObjectOutputStreamPtr GetStream2() const override
			{ if (!_output) throw std::runtime_error("no data stream"); return _output; }

			void OnStream1Exhausted() override
			{
				InputStream stream(_header->GetData());
				Response header;
				header.Read(stream);
				if (_transaction && _transaction != header.Transaction)
				{
					error("drop message ", hex(header.ContainerType, 4), ", response: ", hex(header.ResponseType, 4), ", transaction: ", hex(header.Transaction, 8), ", transaction: ", hex(_transaction, 8));
					_valid = false;
					_output = std::make_shared<DummyOutputStream>();
					return;
				}

				switch(header.ContainerType)
				{
				case ContainerType::Data:
					_output			= _dataOutput;
					break;
				case ContainerType::Response:
					_output			= _response;
					_responseCode	= header.ResponseType;
					_finished		= true;
					break;
				default:
					_valid			= false;
					_output			= std::make_shared<DummyOutputStream>();
				}
			}

		public:
			HeaderParserObjectOutputStream(u32 transaction, IObjectOutputStreamPtr dataOutput):
				_offset(0), _transaction(transaction),
				_header(new FixedSizeByteArrayObjectOutputStream(Response::Size)),
				_response(new ByteArrayObjectOutputStream),
				_dataOutput(dataOutput),
				_valid(true), _finished(false) { }

			ResponseType GetResponseCode() const
			{ return _responseCode; }

			const ByteArray &GetResponse() const
			{ return _response->GetData(); }

			bool Valid() const
			{ return _valid; }
			bool Finished() const
			{ return _finished; }

			size_t Write(const u8 *data, size_t size) override
			{
				size_t r = JoinedObjectOutputStreamBase::Write(data, size);
				_offset += r;
				if (!_stream1Exhausted && _offset >= _header->GetData().size()) {
					_stream1Exhausted = true;
					OnStream1Exhausted();
				}
				return r;
			}
		};
		DECLARE_PTR(HeaderParserObjectOutputStream);
	}

	void PipePacketer::Read(u32 transaction, const IObjectOutputStreamPtr &object, ResponseType &code, ByteArray &response, int timeout)
	{
		response.clear();

		HeaderParserObjectOutputStreamPtr parser;
		MessageParsingStreamPtr output;

		while(true)
		{
			if (!parser)
				parser.reset(new HeaderParserObjectOutputStream(transaction, object));
			if (!output)
				output.reset(new MessageParsingStream(parser));

			ReadMessage(output, timeout);
			if (parser->Finished())
			{
				response = parser->GetResponse();
				code = parser->GetResponseCode();
				break;
			}

			if (output->Finished()) {
				parser.reset();
				output.reset();
			}
		}

		//HexDump("response", response);
	}

	void PipePacketer::Read(u32 transaction, ByteArray &data, ResponseType &code, ByteArray &response, int timeout)
	{
		ByteArrayObjectOutputStreamPtr stream(new ByteArrayObjectOutputStream);
		Read(transaction, stream, code, response, timeout);
		data = stream->GetData();
	}

	void PipePacketer::Abort(u32 transaction, int timeout)
	{
		_pipe->Cancel();
		OperationRequest req(OperationCode::CancelTransaction, transaction);
		HexDump("abort control message", req.Data);
		/* 0x21: host-to-device, class specific, recipient - interface, 0x64: cancel request */
		_pipe->GetDevice()->WriteControl(
			(u8)(usb::RequestType::HostToDevice | usb::RequestType::Class | usb::RequestType::Interface),
			0x64,
			0, _pipe->GetInterface()->GetIndex(), req.Data, timeout);
	}


}

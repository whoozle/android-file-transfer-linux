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
#include <mtp/ptp/ByteArrayObjectStream.h>
#include <mtp/ptp/JoinedObjectStream.h>


namespace mtp
{

	void PipePacketer::Write(const IObjectInputStreamPtr &inputStream, int timeout)
	{ _pipe->Write(inputStream, timeout); }

	void PipePacketer::Write(const ByteArray &data, int timeout)
	{ Write(std::make_shared<ByteArrayObjectInputStream>(data), timeout); }

	namespace
	{
		class MessageParsingStream : public JoinedObjectOutputStreamBase
		{
			FixedSizeByteArrayObjectOutputStreamPtr	_header;
			IObjectOutputStreamPtr					_stream;
			u64										_offset;
			u64										_size;

		private:
			IObjectOutputStreamPtr GetStream1() const
			{ return _header; }

			IObjectOutputStreamPtr GetStream2() const
			{ return _stream; }

		public:
			MessageParsingStream(IObjectOutputStreamPtr stream): _header(new FixedSizeByteArrayObjectOutputStream(4)), _stream(stream), _offset(0), _size(4) { }

			virtual void OnStream1Exhausted()
			{
				_stream1Exhausted = true;
				InputStream is(_header->GetData());
				u32 size;
				is >> size;
				if (size < 4)
					throw std::runtime_error("invalid size/malformed message");
				_size = size;
			}

			virtual size_t Write(const u8 *data, size_t size)
			{
				size_t r = JoinedObjectOutputStreamBase::Write(data, size);
				_offset += r;
				if (_offset == _header->GetData().size())
					OnStream1Exhausted();
				return r;
			}
		};
		DECLARE_PTR(MessageParsingStream);
	}

	void PipePacketer::ReadMessage(const IObjectOutputStreamPtr &outputStream, int timeout)
	{
		MessageParsingStreamPtr output(new MessageParsingStream(outputStream));
		_pipe->Read(output, timeout);
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

	namespace
	{
		struct DummyOutputStream : IObjectOutputStream
		{
			virtual size_t Write(const u8 *data, size_t size)
			{ return size; }
		};

		class HeaderParserObjectOutputStream : public JoinedObjectOutputStreamBase
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
			virtual IObjectOutputStreamPtr GetStream1() const
			{ return _header; }
			virtual IObjectOutputStreamPtr GetStream2() const
			{ if (!_output) throw std::runtime_error("no data stream"); return _output; }

			virtual void OnStream1Exhausted()
			{
				_stream1Exhausted = true;
				InputStream stream(_header->GetData());
				Response header;
				header.Read(stream);
				if (_transaction && _transaction != header.Transaction)
				{
					fprintf(stderr, "drop message %04x %04x, transaction %08x, expected: %08x\n", header.ContainerType, header.ResponseType, header.Transaction, _transaction);
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

			virtual size_t Write(const u8 *data, size_t size)
			{
				size_t r = JoinedObjectOutputStreamBase::Write(data, size);
				_offset += r;
				if (_offset == _header->GetData().size())
					OnStream1Exhausted();
				return r;
			}
		};
		DECLARE_PTR(HeaderParserObjectOutputStream);
	}

	void PipePacketer::Read(u32 transaction, const IObjectOutputStreamPtr &object, ResponseType &code, ByteArray &response, int timeout)
	{
		try
		{ PollEvent(); }
		catch(const std::exception &ex)
		{ fprintf(stderr, "exception in interrupt: %s\n", ex.what()); }

		response.clear();

		while(true)
		{
			HeaderParserObjectOutputStreamPtr parser(new HeaderParserObjectOutputStream(transaction, object));
			ReadMessage(parser, timeout);
			if (parser->Finished())
			{
				response = parser->GetResponse();
				code = parser->GetResponseCode();
				break;
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



}

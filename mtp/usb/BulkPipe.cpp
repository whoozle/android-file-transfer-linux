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

#include <mtp/usb/BulkPipe.h>
#include <usb/DeviceDescriptor.h>
#include <mtp/usb/TimeoutException.h>
#include <mtp/ptp/ByteArrayObjectStream.h>
#include <mtp/log.h>

namespace mtp { namespace usb
{
	BulkPipe::BulkPipe(DevicePtr device, ConfigurationPtr conf, InterfacePtr interface, EndpointPtr in, EndpointPtr out, EndpointPtr interrupt, ITokenPtr claimToken, bool clearHalt):
		_device(device), _conf(conf), _interface(interface), _in(in), _out(out), _interrupt(interrupt), _claimToken(claimToken)
	{
		if (clearHalt)
		{
			try
			{ device->ClearHalt(_interrupt); }
			catch(const std::exception & ex)
			{ error("clearing halt for in ep: ", ex.what()); }

			try
			{ device->ClearHalt(in); }
			catch(const std::exception & ex)
			{ error("clearing halt for in ep: ", ex.what()); }

			try
			{ device->ClearHalt(out); }
			catch(const std::exception & ex)
			{ error("clearing halt for in ep: ", ex.what()); }
		}
	}

	BulkPipe::~BulkPipe()
	{ }

	DevicePtr BulkPipe::GetDevice() const
	{ return _device; }

	InterfacePtr BulkPipe::GetInterface() const
	{ return _interface; }

	ByteArray BulkPipe::ReadInterrupt(int timeout)
	{
		ByteArrayObjectOutputStreamPtr s(new ByteArrayObjectOutputStream());
		try { _device->ReadBulk(_interrupt, s, timeout); } catch(const TimeoutException &ex) { return ByteArray(); }
		return s->GetData();
	}

	void BulkPipe::SetCurrentStream(const ICancellableStreamPtr &stream)
	{
		scoped_mutex_lock l(_mutex);
		_currentStream = stream;
	}

	ICancellableStreamPtr BulkPipe::GetCurrentStream()
	{
		scoped_mutex_lock l(_mutex);
		return _currentStream;
	}


	class BulkPipe::CurrentStreamSetter
	{
		BulkPipe *					_owner;

	public:
		CurrentStreamSetter(BulkPipe *owner, ICancellableStreamPtr stream): _owner(owner)
		{ _owner->SetCurrentStream(stream); }
		~CurrentStreamSetter()
		{ _owner->SetCurrentStream(nullptr); }
	};

	void BulkPipe::Read(const IObjectOutputStreamPtr &outputStream, int timeout)
	{
		CurrentStreamSetter s(this, std::dynamic_pointer_cast<ICancellableStream>(outputStream));
		_device->ReadBulk(_in, outputStream, timeout);
	}

	void BulkPipe::Write(const IObjectInputStreamPtr &inputStream, int timeout)
	{
		CurrentStreamSetter s(this, std::dynamic_pointer_cast<ICancellableStream>(inputStream));
		_device->WriteBulk(_out, inputStream, timeout);
	}

	void BulkPipe::Cancel()
	{
		ICancellableStreamPtr stream = GetCurrentStream();
		print("cancelling stream ", stream.get());
		if (stream)
			stream->Cancel();
	}

	BulkPipePtr BulkPipe::Create(const usb::DevicePtr & device, const ConfigurationPtr & conf, const usb::InterfacePtr & interface, ITokenPtr claimToken)
	{
		int epn = interface->GetEndpointsCount();

		usb::EndpointPtr out, in, interrupt;
		//debug("endpoints: ", epn);
		for(int i = 0; i < epn; ++i)
		{
			usb::EndpointPtr ep = interface->GetEndpoint(i);
			//debug("endpoint: %d: %02x", i, ep->GetAddress());
			//check for bulk here
			if (ep->GetDirection() == usb::EndpointDirection::Out)
			{
				if (ep->GetType() == usb::EndpointType::Bulk)
				{
					//debug("OUT");
					out = ep;
				}
			}
			else
			{
				if (ep->GetType() == usb::EndpointType::Bulk)
				{
					//debug("IN");
					in = ep;
				}
				else
				{
					//debug("INTERRUPT");
					interrupt = ep;
				}
			}
		}
		if (!in || !out || !interrupt)
			throw std::runtime_error("invalid endpoint");

		return std::make_shared<BulkPipe>(device, conf, interface, in, out, interrupt, claimToken);
	}

}}

/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015  Vladimir Menshakov

    Android File Transfer For Linux is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    Android File Transfer For Linux is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Android File Transfer For Linux.
    If not, see <http://www.gnu.org/licenses/>.
 */

#include <mtp/usb/BulkPipe.h>
#include <usb/DeviceDescriptor.h>
#include <mtp/usb/TimeoutException.h>
#include <mtp/ptp/ByteArrayObjectStream.h>

namespace mtp { namespace usb
{
	BulkPipe::BulkPipe(DevicePtr device, ConfigurationPtr conf, InterfacePtr interface, EndpointPtr in, EndpointPtr out, EndpointPtr interrupt, ITokenPtr claimToken):
		_device(device), _conf(conf), _interface(interface), _in(in), _out(out), _interrupt(interrupt), _claimToken(claimToken)
	{
		int currentConfigurationIndex = _device->GetConfiguration();
		if (conf->GetIndex() != currentConfigurationIndex)
			_device->SetConfiguration(conf->GetIndex());
	}

	BulkPipe::~BulkPipe()
	{ }

	DevicePtr BulkPipe::GetDevice() const
	{ return _device; }

	ByteArray BulkPipe::ReadInterrupt()
	{
#if 0
#warning causes out of memory errors in some time
		ByteArrayObjectOutputStreamPtr s(new ByteArrayObjectOutputStream());
		try { _device->ReadBulk(_interrupt, s, 0); } catch(const TimeoutException &ex) { return ByteArray(); }
		return std::move(s->GetData());
#else
		return ByteArray();
#endif
	}

	void BulkPipe::Read(const IObjectOutputStreamPtr &outputStream, int timeout)
	{
		_device->ReadBulk(_in, outputStream, timeout);
	}

	void BulkPipe::Write(const IObjectInputStreamPtr &inputStream, int timeout)
	{
		_device->WriteBulk(_out, inputStream, timeout);
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

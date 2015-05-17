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
#include <mtp/usb/BulkPipe.h>
#include <usb/DeviceDescriptor.h>
#include <mtp/usb/TimeoutException.h>
#include <mtp/ptp/ByteArrayObjectStream.h>

namespace mtp { namespace usb
{
	BulkPipe::BulkPipe(DevicePtr device, ConfigurationPtr conf, InterfacePtr interface, EndpointPtr in, EndpointPtr out, EndpointPtr interrupt):
		_device(device), _conf(conf), _interface(interface), _in(in), _out(out), _interrupt(interrupt)
	{
		int currentConfigurationIndex = _device->GetConfiguration();
		if (conf->GetIndex() != currentConfigurationIndex)
			_device->SetConfiguration(conf->GetIndex());

		_claimToken = _device->ClaimInterface(interface->GetIndex());
	}

	BulkPipe::~BulkPipe()
	{ }

	DevicePtr BulkPipe::GetDevice() const
	{ return _device; }

	ByteArray BulkPipe::ReadInterrupt()
	{
		ByteArrayObjectOutputStreamPtr s(new ByteArrayObjectOutputStream());
		try { _device->ReadBulk(_interrupt, s, 0); } catch(const TimeoutException &ex) { return ByteArray(); }
		return std::move(s->GetData());
	}

	void BulkPipe::Read(const IObjectOutputStreamPtr &outputStream, int timeout)
	{
		_device->ReadBulk(_in, outputStream, timeout);
	}

	void BulkPipe::Write(const IObjectInputStreamPtr &inputStream, int timeout)
	{
		_device->WriteBulk(_out, inputStream, timeout);
	}

	BulkPipePtr BulkPipe::Create(const usb::DevicePtr & device, const ConfigurationPtr & conf, const usb::InterfacePtr & interface)
	{
		int epn = interface->GetEndpointsCount();

		usb::EndpointPtr out, in, interrupt;
		//fprintf(stderr, "endpoints: %d\n", epn);
		for(int i = 0; i < epn; ++i)
		{
			usb::EndpointPtr ep = interface->GetEndpoint(i);
			//fprintf(stderr, "endpoint: %d: %02x\n", i, ep->GetAddress());
			//check for bulk here
			if (ep->GetDirection() == usb::EndpointDirection::Out)
			{
				if (ep->GetType() == usb::EndpointType::Bulk)
				{
					//fprintf(stderr, "OUT\n");
					out = ep;
				}
			}
			else
			{
				if (ep->GetType() == usb::EndpointType::Bulk)
				{
					//fprintf(stderr, "IN\n");
					in = ep;
				}
				else
				{
					//fprintf(stderr, "INTERRUPT\n");
					interrupt = ep;
				}
			}
		}
		if (!in || !out || !interrupt)
			throw std::runtime_error("invalid endpoint");

		return std::make_shared<BulkPipe>(device, conf, interface, in, out, interrupt);
	}

}}

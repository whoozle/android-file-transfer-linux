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
#include <usb/Device.h>
#include <usb/Context.h>
#include <usb/Interface.h>
#include <usb/call.h>
#include <mtp/ByteArray.h>

#include <usb/call.h>

namespace mtp { namespace usb
{

	Endpoint::Endpoint(IOUSBInterfaceInterface **interface, int idx)
	{
		UInt8		direction;
		UInt8		number;
		UInt8		transferType;
		UInt16		maxPacketSize;
		UInt8		interval;

		USB_CALL((*interface)->GetPipeProperties(interface,
			idx, &direction,
			&number, &transferType,
			&maxPacketSize, &interval));

		switch (direction)
		{
		case kUSBOut:	_direction = EndpointDirection::Out; break;
		case kUSBIn:	_direction = EndpointDirection::In; break;
		default:		throw std::runtime_error("invalid endpoint direction");
		}

		_address = number;

		switch(transferType)
		{
		case kUSBControl:	_type = EndpointType::Control; break;
		case kUSBIsoc:		_type = EndpointType::Isochronous; break;
		case kUSBBulk:		_type = EndpointType::Bulk; break;
		case kUSBInterrupt:	_type = EndpointType::Interrupt; break;
		default:		throw std::runtime_error("invalid endpoint type");
		}

		_maxPacketSize = maxPacketSize;
	}


	Device::Device(ContextPtr context, IOUSBDeviceInterface ** dev): _context(context), _dev(dev)
	{ }

	Device::~Device()
	{
		(*_dev)->USBDeviceClose(_dev);
	}

	int Device::GetConfiguration() const
	{ return 0; }

	void Device::SetConfiguration(int idx)
	{ }

	void Device::WriteBulk(const EndpointPtr & ep, const IObjectInputStreamPtr &inputStream, int timeout)
	{ }

	void Device::ReadBulk(const EndpointPtr & ep, const IObjectOutputStreamPtr &outputStream, int timeout)
	{ }

	void Device::WriteControl(u8 type, u8 req, u16 value, u16 index, const ByteArray &data, bool interruptCurrentTransaction, int timeout)
	{
	}

	InterfaceTokenPtr Device::ClaimInterface(const InterfacePtr &interface)
	{ return interface->Claim(); }

}}
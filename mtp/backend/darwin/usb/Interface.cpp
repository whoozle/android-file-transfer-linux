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

#include <usb/Interface.h>
#include <usb/Device.h>
#include <usb/call.h>

namespace mtp { namespace usb
{

	Endpoint::Endpoint(IOUSBInterfaceInterface **interface, int idx): _interface(interface), _refIndex(idx)
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
		case kUSBOut:		_direction = EndpointDirection::Out; break;
		case kUSBIn:		_direction = EndpointDirection::In; break;
		case kUSBAnyDirn:	_direction = EndpointDirection::Both; break;
		default:			throw std::runtime_error("invalid endpoint direction");
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

	Interface::Interface(DevicePtr device, ConfigurationPtr config, IOUSBInterfaceInterface **interface): _device(device), _config(config), _interface(interface) { }

	Interface::~Interface()
	{ }

	u8 Interface::GetClass() const
	{
		UInt8 intfClass;
		USB_CALL((*_interface)->GetInterfaceClass(_interface, &intfClass));
		return intfClass;
	}

	u8 Interface::GetSubclass() const
	{
		UInt8 intfSubClass;
		USB_CALL((*_interface)->GetInterfaceSubClass(_interface, &intfSubClass));
		return intfSubClass;
	}

	int Interface::GetIndex() const
	{
		UInt8 intfNumber;
		USB_CALL((*_interface)->GetInterfaceNumber(_interface, &intfNumber));
		return intfNumber;
	}

	int Interface::GetEndpointsCount() const
	{
		UInt8 intfNumEndpoints;
		USB_CALL((*_interface)->GetNumEndpoints(_interface, &intfNumEndpoints));
		return intfNumEndpoints;
	}

	InterfaceToken::InterfaceToken(IOUSBInterfaceInterface **interface): _interface(interface)
	{
		USB_CALL((*_interface)->USBInterfaceOpen(_interface));
	}

	InterfaceToken::~InterfaceToken()
	{  (*_interface)->USBInterfaceClose(_interface); }

	InterfaceTokenPtr Interface::Claim()
	{ return std::make_shared<InterfaceToken>(_interface); }


}}

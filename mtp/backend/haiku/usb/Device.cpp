/*
 * Device.cpp
 * Copyright (C) 2022 pulkomandy <pulkomandy@shredder>
 *
 * Distributed under terms of the MIT license.
 */

#include "Device.h"


namespace mtp { namespace usb
{

Device::Device(ContextPtr ctx, BUSBDevice* handle)
	: _context(ctx)
	, _handle(handle)
{
}

Device::~Device()
{
}

void Device::Reset()
{
}

void Device::ReadControl(u8 type, u8 req, u16 value, u16 index, ByteArray &data, int timeout)
{
	fprintf(stderr, "Control read(type %x req %x value %x index %d size %lu) with timeout %d…",
		type, req, value, index, data.size(), timeout);
	ssize_t result = _handle->ControlTransfer(type, req, value, index, data.size(), const_cast<u8*>(data.data()));
	if (result >= 0)
		data.resize(result);
	else
		throw std::runtime_error("read fail");
	fprintf(stderr, " complete (%ld).\n", result);
}

void Device::WriteControl(u8 type, u8 req, u16 value, u16 index, const ByteArray &data, int timeout)
{
	fprintf(stderr, "Control write with timeout %d\n", timeout);
	_handle->ControlTransfer(USB_REQTYPE_DEVICE_IN, req, value, index, data.size(), const_cast<u8*>(data.data()));
}

void Device::SetConfiguration(int idx)
{
	_handle->SetConfiguration(_handle->ConfigurationAt(idx));
}

void Device::WriteBulk(const EndpointPtr & ep, const IObjectInputStreamPtr &inputStream, int timeout)
{
	ByteArray data(inputStream->GetSize());
	inputStream->Read(data.data(), data.size());
	ssize_t tr = ep->_endpoint.BulkTransfer(data.data(), data.size());
	if (tr != (int)data.size())
		throw std::runtime_error("short write");
}

void Device::ReadBulk(const EndpointPtr & ep, const IObjectOutputStreamPtr &outputStream, int timeout)
{
	ByteArray data(ep->GetMaxPacketSize());
	ssize_t tr;
	{
		tr = ep->_endpoint.BulkTransfer(data.data(), data.size());
		outputStream->Write(data.data(), tr);
	}
	while(tr == (int)data.size());
}

void Device::ClearHalt(const EndpointPtr & ep)
{
	ep->_endpoint.ClearStall();
}

InterfaceTokenPtr Device::ClaimInterface(const InterfacePtr & interface)
{
	return InterfaceTokenPtr();
}

}}

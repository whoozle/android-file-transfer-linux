/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2018  Vladimir Menshakov

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

#include <usb/Device.h>
#include <usb/Context.h>
#include <usb/Interface.h>
#include <usb/call.h>
#include <mtp/ByteArray.h>
#include <mtp/log.h>

namespace mtp { namespace usb
{

	Device::Device(ContextPtr context, IOUSBDeviceType ** dev): _context(context), _dev(dev)
	{ }

	Device::~Device()
	{
		(*_dev)->USBDeviceClose(_dev);
	}

	int Device::GetConfiguration() const
	{ return 0; }

	void Device::SetConfiguration(int idx)
	{ }

	void Device::ClearHalt(const EndpointPtr & ep)
	{ }

	void Device::WriteBulk(const EndpointPtr & ep, const IObjectInputStreamPtr &inputStream, int timeout)
	{
		IOUSBInterfaceInterface	** interface = ep->GetInterfaceHandle();

		size_t transferSize = ep->GetMaxPacketSize();
		ByteArray buffer(transferSize);
		size_t r;
		do
		{
			r = inputStream->Read(buffer.data(), buffer.size());
			USB_CALL((*interface)->WritePipe(interface, ep->GetRefIndex(), buffer.data(), r));
		}
		while(r == transferSize);
	}

	void Device::ReadBulk(const EndpointPtr & ep, const IObjectOutputStreamPtr &outputStream, int timeout)
	{
		IOUSBInterfaceInterface	** interface = ep->GetInterfaceHandle();
		size_t transferSize = ep->GetMaxPacketSize();
		ByteArray buffer(transferSize);
		size_t r;
		do
		{
			UInt32 readBytes = buffer.size();
			USB_CALL((*interface)->ReadPipe(interface, ep->GetRefIndex(), buffer.data(), &readBytes));
			r = outputStream->Write(buffer.data(), readBytes);
		}
		while(r == transferSize);
	}

	void Device::ReadControl(IOUSBDeviceType **dev, u8 type, u8 req, u16 value, u16 index, ByteArray &data, int timeout)
	{
		IOUSBDevRequest request = {};
		request.bmRequestType = type;
		request.bRequest = req;
		request.wValue = value;
		request.wIndex = index;
		request.pData = data.data();
		request.wLength = data.size();
		USB_CALL((*dev)->DeviceRequest(dev, &request));
		data.resize(request.wLenDone);
	}

	void Device::ReadControl(u8 type, u8 req, u16 value, u16 index, ByteArray &data, int timeout)
	{ ReadControl(_dev, type, req, value, index, data, timeout); }

	void Device::WriteControl(u8 type, u8 req, u16 value, u16 index, const ByteArray &data, int timeout)
	{ error("WriteControl stub"); }

	InterfaceTokenPtr Device::ClaimInterface(const InterfacePtr &interface)
	{ return interface->Claim(); }

}}

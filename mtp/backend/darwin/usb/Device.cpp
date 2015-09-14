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
	{
		IOUSBInterfaceInterface	** interface = ep->GetInterfaceHandle();

		size_t transferSize = ep->GetMaxPacketSize();
		ByteArray buffer(transferSize);
		size_t r;
		do
		{
			r = inputStream->Read(buffer.data(), buffer.size());
			USB_CALL((*interface)->WritePipe(interface, ep->GetRefIndex(), buffer.data(), buffer.size()));
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

	void Device::ReadControl(u8 type, u8 req, u16 value, u16 index, ByteArray &data, int timeout)
	{
		IOUSBDevRequest req = {};
		req.rqDirection = kUSBIn;
		req.rqType = (type >> 5) & 0x03;
		req.rqRecipient = type & 0x1f;
		req.bRequest = req;
		req.wValue = value;
		req.wIndex = index;
		req.pData = data.data();
		req.wLength = data.size();
		USB_CALL((*_dev)->DeviceRequest(_dev, &req));
		data.resize(req.wLenDone);
	}

	void Device::WriteControl(u8 type, u8 req, u16 value, u16 index, const ByteArray &data, bool interruptCurrentTransaction, int timeout)
	{ fprintf(stderr, "WriteControl stub\n"); }

	InterfaceTokenPtr Device::ClaimInterface(const InterfacePtr &interface)
	{ return interface->Claim(); }

}}
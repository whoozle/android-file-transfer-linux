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

	void Device::Reset()
	{ }

	int Device::GetConfiguration() const
	{ return 0; }

	void Device::SetConfiguration(int idx)
	{ USB_CALL((*_dev)->SetConfiguration(_dev, idx)); }

	void Device::ClearHalt(const EndpointPtr & ep)
	{
		IOUSBInterfaceType	** interface = ep->GetInterfaceHandle();
		USB_CALL((*interface)->ClearPipeStall(interface, ep->GetRefIndex()));
	}

	void Device::WriteBulk(const EndpointPtr & ep, const IObjectInputStreamPtr &inputStream, int timeout)
	{
		IOUSBInterfaceType	** interface = ep->GetInterfaceHandle();

		static constexpr size_t MaxBulkTransferSize = 256 * 1024;
		size_t maxPacket = ep->GetMaxPacketSize();
		if (maxPacket == 0)
			maxPacket = 512;
		size_t transferSize = std::max(maxPacket, (MaxBulkTransferSize / maxPacket) * maxPacket);
		ByteArray buffer(transferSize);
		size_t r;
		UInt32 noDataTimeout = timeout > 0 ? timeout : 0;
		UInt32 completionTimeout = timeout > 0 ? std::max(timeout, 30000) : 0;

		do
		{
			r = inputStream->Read(buffer.data(), buffer.size());
			USB_CALL((*interface)->WritePipeTO(interface, ep->GetRefIndex(), buffer.data(), static_cast<UInt32>(r), noDataTimeout, completionTimeout));
		}
		while(r == transferSize);

		if (r > 0 && (r % maxPacket == 0))
		{
			USB_CALL((*interface)->WritePipeTO(interface, ep->GetRefIndex(), buffer.data(), 0, noDataTimeout, completionTimeout));
		}
	}

	void Device::ReadBulk(const EndpointPtr & ep, const IObjectOutputStreamPtr &outputStream, int timeout)
	{
		IOUSBInterfaceType	** interface = ep->GetInterfaceHandle();
		static constexpr size_t MaxBulkTransferSize = 256 * 1024;
		size_t maxPacket = ep->GetMaxPacketSize();
		if (maxPacket == 0)
			maxPacket = 512;
		size_t transferSize = std::max(maxPacket, (MaxBulkTransferSize / maxPacket) * maxPacket);
		ByteArray buffer(transferSize);
		size_t r;
		UInt32 noDataTimeout = timeout > 0 ? timeout : 0;
		UInt32 completionTimeout = timeout > 0 ? std::max(timeout, 30000) : 0;

		do
		{
			UInt32 readBytes = static_cast<UInt32>(buffer.size());
			USB_CALL((*interface)->ReadPipeTO(interface, ep->GetRefIndex(), buffer.data(), &readBytes, noDataTimeout, completionTimeout));
			r = outputStream->Write(buffer.data(), readBytes);
		}
		while(r == transferSize);
	}

	void Device::ReadControl(IOUSBDeviceType **dev, u8 type, u8 req, u16 value, u16 index, ByteArray &data, int timeout)
	{
		UInt32 noDataTimeout = timeout > 0 ? timeout : 0;
		UInt32 completionTimeout = timeout > 0 ? std::max(timeout, 30000) : 0;

		IOUSBDevRequestTO request = {};
		request.bmRequestType = type;
		request.bRequest = req;
		request.wValue = value;
		request.wIndex = index;
		request.pData = data.data();
		request.wLength = static_cast<UInt16>(data.size());
		request.noDataTimeout = noDataTimeout;
		request.completionTimeout = completionTimeout;
		USB_CALL((*dev)->DeviceRequestTO(dev, &request));
		data.resize(request.wLenDone);
	}

	void Device::ReadControl(u8 type, u8 req, u16 value, u16 index, ByteArray &data, int timeout)
	{ ReadControl(_dev, type, req, value, index, data, timeout); }

	void Device::WriteControl(u8 type, u8 req, u16 value, u16 index, const ByteArray &data, int timeout)
	{
		UInt32 noDataTimeout = timeout > 0 ? timeout : 0;
		UInt32 completionTimeout = timeout > 0 ? std::max(timeout, 30000) : 0;

		IOUSBDevRequestTO request = {};
		request.bmRequestType = type;
		request.bRequest = req;
		request.wValue = value;
		request.wIndex = index;
		request.pData = const_cast<u8 *>(data.data());
		request.wLength = static_cast<UInt16>(data.size());
		request.noDataTimeout = noDataTimeout;
		request.completionTimeout = completionTimeout;
		USB_CALL((*_dev)->DeviceRequestTO(_dev, &request));
	}

	InterfaceTokenPtr Device::ClaimInterface(const InterfacePtr &interface)
	{ return interface->Claim(); }

}}

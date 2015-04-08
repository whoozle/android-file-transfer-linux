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
#include <usb/call.h>
#include <mtp/ByteArray.h>

namespace mtp { namespace usb
{

	Device::Device(ContextPtr context, libusb_device_handle * handle): _context(context), _handle(handle)
	{}

	Device::~Device()
	{
		libusb_close(_handle);
	}

	int Device::GetConfiguration() const
	{
		int config;
		USB_CALL(libusb_get_configuration(_handle, &config));
		return config;
	}

	void Device::SetConfiguration(int idx)
	{
		USB_CALL(libusb_set_configuration(_handle, idx));
	}

	Device::InterfaceToken::InterfaceToken(libusb_device_handle *handle, int index): _handle(handle), _index(index)
	{
		USB_CALL(libusb_claim_interface(handle, index));
	}

	Device::InterfaceToken::~InterfaceToken()
	{ libusb_release_interface(_handle, _index); }

	void Device::WriteBulk(const EndpointPtr & ep, const IObjectInputStreamPtr &inputStream, int timeout)
	{
#if 0
		//triggers short read exception on first write.
		size_t transferSize = ep->GetMaxPacketSize() * 1024;
		size_t r;
		do
		{
			ByteArray data(transferSize);
			r = inputStream->Read(data.data(), data.size());
			int tr = 0;
			USB_CALL(libusb_bulk_transfer(_handle, ep->GetAddress(), data.data(), r, &tr, timeout));
			if (tr != (int)r)
				throw std::runtime_error("short write");
		}
		while(r == transferSize);
#endif
		ByteArray data(inputStream->GetSize());
		inputStream->Read(data.data(), data.size());
		int tr = 0;
		USB_CALL(libusb_bulk_transfer(_handle, ep->GetAddress(), const_cast<u8 *>(data.data()), data.size(), &tr, timeout));
		if (tr != (int)data.size())
			throw std::runtime_error("short write");
	}

	void Device::ReadBulk(const EndpointPtr & ep, const IObjectOutputStreamPtr &outputStream, int timeout)
	{
		ByteArray data(ep->GetMaxPacketSize() * 1024);
		int tr = 0;
		USB_CALL(libusb_bulk_transfer(_handle, ep->GetAddress(), data.data(), data.size(), &tr, timeout));
		outputStream->Write(data.data(), tr);
	}


	std::string Device::GetString(int idx) const
	{
		unsigned char buffer[4096];
		int r = libusb_get_string_descriptor_ascii(_handle, idx, buffer, sizeof(buffer));
		if (r < 0)
			throw Exception("libusb_get_string_descriptor_ascii", r);
		return std::string(buffer, buffer + r);
	}

}}
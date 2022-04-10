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

#ifndef AFTL_MTP_BACKEND_LIBUSB_USB_DEVICE_H
#define AFTL_MTP_BACKEND_LIBUSB_USB_DEVICE_H

#include <mtp/ptp/IObjectStream.h>
#include <mtp/ByteArray.h>
#include <mtp/Token.h>
#include <mtp/types.h>
#include <mtp/usb/types.h>
#include <device/USBKit.h>

namespace mtp { namespace usb
{
	class Context;
	DECLARE_PTR(Context);

	class Endpoint
	{
		const BUSBEndpoint & _endpoint;

	public:
		Endpoint(const BUSBEndpoint & endpoint) : _endpoint(endpoint) { }

		u8 GetAddress() const
		{ return _endpoint.Index(); }

		int GetMaxPacketSize() const
		{ return _endpoint.MaxPacketSize(); }

		EndpointDirection GetDirection() const
		{
			if (_endpoint.IsInput())
				return EndpointDirection::In;
			else
				return EndpointDirection::Out;
		}

		EndpointType GetType() const
		{
			if (_endpoint.IsBulk())
				return EndpointType::Bulk;
			if (_endpoint.IsInterrupt())
				return EndpointType::Interrupt;
			if (_endpoint.IsIsochronous())
				return EndpointType::Isochronous;
			return EndpointType::Control;
		}

		friend class Device;
	};
	DECLARE_PTR(Endpoint);

	class Interface;
	DECLARE_PTR(Interface);
	class InterfaceToken;
	DECLARE_PTR(InterfaceToken);

	class Device : Noncopyable
	{
	private:
		ContextPtr				_context;
		BUSBDevice *	_handle;

	public:
		Device(ContextPtr ctx, BUSBDevice * handle);
		~Device();

		BUSBDevice * GetHandle()
		{ return _handle; }

		InterfaceTokenPtr ClaimInterface(const InterfacePtr & interface);

		void Reset();
		int GetConfiguration() const;
		void SetConfiguration(int idx);

		void WriteBulk(const EndpointPtr & ep, const IObjectInputStreamPtr &inputStream, int timeout);
		void ReadBulk(const EndpointPtr & ep, const IObjectOutputStreamPtr &outputStream, int timeout);

		void ReadControl(u8 type, u8 req, u16 value, u16 index, ByteArray &data, int timeout);
		void WriteControl(u8 type, u8 req, u16 value, u16 index, const ByteArray &data, int timeout);

		void ClearHalt(const EndpointPtr & ep);

		std::string GetString(int idx) const;
	};
	DECLARE_PTR(Device);
}}

#endif	/* DEVICE_H */


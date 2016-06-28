/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2016  Vladimir Menshakov

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

#ifndef DEVICE_H
#define	DEVICE_H

#include <mtp/ptp/IObjectStream.h>
#include <mtp/ByteArray.h>
#include <mtp/Token.h>
#include <mtp/types.h>
#include <mtp/usb/types.h>
#include <libusb.h>

namespace mtp { namespace usb
{
	class Context;
	DECLARE_PTR(Context);

	class Endpoint
	{
		const libusb_endpoint_descriptor & _endpoint;

	public:
		Endpoint(const libusb_endpoint_descriptor & endpoint) : _endpoint(endpoint) { }

		u8 GetAddress() const
		{ return _endpoint.bEndpointAddress; }

		int GetMaxPacketSize() const
		{ return _endpoint.wMaxPacketSize; }

		EndpointDirection GetDirection() const
		{
			u8 dir = GetAddress() & LIBUSB_ENDPOINT_DIR_MASK;
			if (dir == LIBUSB_ENDPOINT_IN)
				return EndpointDirection::In;
			else
				return EndpointDirection::Out;
		}

		EndpointType GetType() const
		{
			return EndpointType(_endpoint.bmAttributes & 3);
		}
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
		libusb_device_handle *	_handle;

	public:
		Device(ContextPtr ctx, libusb_device_handle * handle);
		~Device();

		libusb_device_handle * GetHandle()
		{ return _handle; }

		InterfaceTokenPtr ClaimInterface(const InterfacePtr & interface);

		int GetConfiguration() const;
		void SetConfiguration(int idx);

		void WriteBulk(const EndpointPtr & ep, const IObjectInputStreamPtr &inputStream, int timeout);
		void ReadBulk(const EndpointPtr & ep, const IObjectOutputStreamPtr &outputStream, int timeout);

		void ReadControl(u8 type, u8 req, u16 value, u16 index, ByteArray &data, int timeout);
		void WriteControl(u8 type, u8 req, u16 value, u16 index, const ByteArray &data, int timeout);

		std::string GetString(int idx) const;
	};
	DECLARE_PTR(Device);
}}

#endif	/* DEVICE_H */


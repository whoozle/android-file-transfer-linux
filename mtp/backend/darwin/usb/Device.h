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
#ifndef DEVICE_H
#define	DEVICE_H

#include <mtp/ptp/IObjectStream.h>
#include <mtp/ByteArray.h>
#include <mtp/Token.h>
#include <mtp/types.h>

#include <usb/usb.h>

namespace mtp { namespace usb
{
	class Context;
	DECLARE_PTR(Context);

	enum struct EndpointType
	{
		Control = 0, Isochronous = 1, Bulk = 2, Interrupt = 3
	};

	enum struct EndpointDirection
	{
		In, Out
	};


	class Endpoint
	{
	private:
		int							_address;
		int							_maxPacketSize;
		EndpointDirection			_direction;
		EndpointType				_type;

	public:
		Endpoint(IOUSBInterfaceInterface **interface, int idx);

		u8 GetAddress() const
		{ return _address; }

		int GetMaxPacketSize() const
		{ return _maxPacketSize; }

		EndpointDirection GetDirection() const
		{ return _direction; }

		EndpointType GetType() const
		{ return _type; }
	};
	DECLARE_PTR(Endpoint);

	class Device : Noncopyable
	{
	private:
		ContextPtr					_context;
		IOUSBDeviceInterface **		_dev;

	public:
		class InterfaceToken : public IToken
		{
			int						_index;

		public:
			InterfaceToken(int index);
			~InterfaceToken();
		};
		DECLARE_PTR(InterfaceToken);

		Device(ContextPtr ctx, IOUSBDeviceInterface **dev); //must be opened
		~Device();

		InterfaceTokenPtr ClaimInterface(int index)
		{ return std::make_shared<InterfaceToken>(index); }

		int GetConfiguration() const;
		void SetConfiguration(int idx);

		void WriteBulk(const EndpointPtr & ep, const IObjectInputStreamPtr &inputStream, int timeout);
		void ReadBulk(const EndpointPtr & ep, const IObjectOutputStreamPtr &outputStream, int timeout);

		void WriteControl(u8 type, u8 req, u16 value, u16 index, const ByteArray &data, bool interruptCurrentTransaction, int timeout);

		std::string GetString(int idx) const;
	};
	DECLARE_PTR(Device);
}}

#endif	/* DEVICE_H */


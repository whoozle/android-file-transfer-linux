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
#include <mtp/usb/types.h>

#include <usb/usb.h>

namespace mtp { namespace usb
{
	class Context;
	DECLARE_PTR(Context);

	class Device : Noncopyable
	{
	private:
		ContextPtr					_context;
		IOUSBDeviceInterface **		_dev;

	public:
		Device(ContextPtr ctx, IOUSBDeviceInterface **dev); //must be opened
		~Device();

		InterfaceTokenPtr ClaimInterface(const InterfacePtr &interface)
		{ return std::make_shared<InterfaceToken>(interface); }

		int GetConfiguration() const;
		void SetConfiguration(int idx);

		void WriteBulk(const EndpointPtr & ep, const IObjectInputStreamPtr &inputStream, int timeout);
		void ReadBulk(const EndpointPtr & ep, const IObjectOutputStreamPtr &outputStream, int timeout);

		void WriteControl(u8 type, u8 req, u16 value, u16 index, const ByteArray &data, bool interruptCurrentTransaction, int timeout);
	};
	DECLARE_PTR(Device);
}}

#endif	/* DEVICE_H */


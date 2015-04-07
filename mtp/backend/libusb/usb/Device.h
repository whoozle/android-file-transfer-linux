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

#include <mtp/types.h>
#include <libusb.h>

namespace mtp { namespace usb
{
	class Context;
	DECLARE_PTR(Context);

	class Device : Noncopyable
	{
	private:
		ContextPtr				_context;
		libusb_device_handle *	_handle;

	public:
		class InterfaceToken
		{
			libusb_device_handle *	_handle;
			int						_index;

		public:
			InterfaceToken(libusb_device_handle *handle, int index);
			~InterfaceToken();
		};
		DECLARE_PTR(InterfaceToken);

		Device(ContextPtr ctx, libusb_device_handle * handle);
		~Device();

		libusb_device_handle * GetHandle()
		{ return _handle; }

		InterfaceTokenPtr ClaimInterface(int index)
		{ return std::make_shared<InterfaceToken>(_handle, index); }

		int GetConfiguration() const;
		void SetConfiguration(int idx);

		std::string GetString(int idx) const;
	};
	DECLARE_PTR(Device);
}}

#endif	/* DEVICE_H */


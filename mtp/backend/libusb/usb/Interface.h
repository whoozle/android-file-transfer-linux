/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015  Vladimir Menshakov

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

#ifndef INTERFACE_H
#define	INTERFACE_H

#include <mtp/types.h>
#include <usb/Device.h>
#include <libusb.h>

namespace mtp { namespace usb
{

	class Configuration;
	DECLARE_PTR(Configuration);

	class InterfaceToken : public IToken
	{
		libusb_device_handle *	_handle;
		int						_index;

	public:
		InterfaceToken(libusb_device_handle *handle, int index);
		~InterfaceToken();
	};
	DECLARE_PTR(InterfaceToken);

	class Interface
	{
		DevicePtr							_device;
		ConfigurationPtr					_config;
		const libusb_interface_descriptor &	_interface;

	public:
		Interface(DevicePtr device, ConfigurationPtr config, const libusb_interface_descriptor &interface): _device(device), _config(config), _interface(interface) { }

		u8 GetClass() const
		{ return _interface.bInterfaceClass; }

		u8 GetSubclass() const
		{ return _interface.bInterfaceSubClass; }

		int GetIndex() const
		{ return _interface.bInterfaceNumber; }

		EndpointPtr GetEndpoint(int idx) const
		{ return std::make_shared<Endpoint>(_interface.endpoint[idx]); }

		int GetEndpointsCount() const
		{ return _interface.bNumEndpoints; }
	};
	DECLARE_PTR(Interface);

}}

#endif

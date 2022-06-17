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

#ifndef AFTL_MTP_BACKEND_LIBUSB_USB_INTERFACE_H
#define AFTL_MTP_BACKEND_LIBUSB_USB_INTERFACE_H

#include <mtp/types.h>
#include <usb/Device.h>

namespace mtp { namespace usb
{

	class Configuration;
	DECLARE_PTR(Configuration);

	class InterfaceToken : public IToken
	{
		BUSBDevice *	_handle;
		int						_index;

	public:
		InterfaceToken(BUSBDevice *handle, int index);
		~InterfaceToken();
	};
	DECLARE_PTR(InterfaceToken);

	class Interface
	{
		DevicePtr							_device;
		ConfigurationPtr					_config;
		const BUSBInterface &	_interface;

	public:
		Interface(DevicePtr device, ConfigurationPtr config, const BUSBInterface &interface): _device(device), _config(config), _interface(interface) { }

		u8 GetClass() const
		{ return _interface.Class(); }

		u8 GetSubclass() const
		{ return _interface.Subclass(); }

		int GetIndex() const
		{ return _interface.Index(); }

		EndpointPtr GetEndpoint(int idx) const
		{ return std::make_shared<Endpoint>(*_interface.EndpointAt(idx)); }

		int GetEndpointsCount() const
		{ return _interface.CountEndpoints(); }

		std::string GetName() const
		{ return _interface.InterfaceString(); }
	};
	DECLARE_PTR(Interface);

}}

#endif

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

#ifndef AFTL_MTP_BACKEND_LINUX_USB_INTERFACE_H
#define AFTL_MTP_BACKEND_LINUX_USB_INTERFACE_H

#include <mtp/types.h>
#include <mtp/usb/types.h>
#include <usb/Endpoint.h> //remove me later, when Endpoint.h will be available in all ports
#include <vector>

namespace mtp { namespace usb
{

	class Endpoint;
	DECLARE_PTR(Endpoint);

	class Configuration;
	DECLARE_PTR(Configuration);

	class Interface
	{
		std::string					_path;
		std::vector<EndpointPtr>	_endpoints;
		u8							_class;
		u8							_subclass;
		int							_index;


	public:
		Interface(int index, const std::string &path);

		u8 GetClass() const
		{ return _class; }

		u8 GetSubclass() const
		{ return _subclass; }

		int GetIndex() const
		{ return _index; }

		EndpointPtr GetEndpoint(int idx) const
		{ return _endpoints.at(idx); }

		int GetEndpointsCount() const
		{ return _endpoints.size(); }
	};
	DECLARE_PTR(Interface);

}}

#endif

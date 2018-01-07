/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2018  Vladimir Menshakov

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

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
#ifndef INTERFACE_H
#define	INTERFACE_H

#include <mtp/types.h>
#include <vector>

namespace mtp { namespace usb
{
	enum struct EndpointType
	{
		Control = 0, Isochronous = 1, Bulk = 2, Interrupt = 3
	};

	enum struct EndpointDirection
	{
		In, Out, Both
	};


	class Endpoint;
	DECLARE_PTR(Endpoint);

	class Endpoint
	{
		EndpointDirection	_direction;
		EndpointType		_type;
		u8					_addr;
		u16					_maxPacketSize;

	public:
		Endpoint(const std::string &path);

		u8 GetAddress() const
		{ return _addr; }

		int GetMaxPacketSize() const
		{ return _maxPacketSize; }

		EndpointDirection GetDirection() const
		{ return _direction; }

		EndpointType GetType() const
		{ return _type; }

		static EndpointPtr TryOpen(const std::string &path);
	};
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
		std::string					_name;


	public:
		Interface(int index, const std::string &path);

		u8 GetClass() const
		{ return _class; }

		u8 GetSubclass() const
		{ return _subclass; }

		int GetIndex() const
		{ return _index; }

		const std::string GetName() const
		{ return _name; }

		EndpointPtr GetEndpoint(int idx) const
		{ return _endpoints.at(idx); }

		int GetEndpointsCount() const
		{ return _endpoints.size(); }
	};
	DECLARE_PTR(Interface);

}}

#endif

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

namespace mtp { namespace usb
{
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
		EndpointDirection	_direction;
		EndpointType		_type;

	public:
		Endpoint() { }

		u8 GetAddress() const
		{ return 0; }

		int GetMaxPacketSize() const
		{ return 0; }

		EndpointDirection GetDirection() const
		{ return _direction; }

		EndpointType GetType() const
		{ return _type; }
	};
	DECLARE_PTR(Endpoint);

	class Configuration;
	DECLARE_PTR(Configuration);

	class Interface
	{
		std::string _path;

	public:
		Interface(int index, const std::string &path): _path(path)
		{ }

		u8 GetClass() const
		{ return 0; }

		u8 GetSubclass() const
		{ return 0; }

		int GetIndex() const
		{ return 0; }

		int GetNameIndex() const
		{ return 0; }

		EndpointPtr GetEndpoint(int idx) const
		{ return std::make_shared<Endpoint>(); }

		int GetEndpointsCount() const
		{ return 0; }
	};
	DECLARE_PTR(Interface);

}}

#endif

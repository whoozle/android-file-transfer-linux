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

#ifndef USBFS_ENDPOINT_H
#define USBFS_ENDPOINT_H

#include <mtp/types.h>
#include <mtp/usb/types.h>
#include <vector>

namespace mtp { namespace usb
{

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

}}

#endif

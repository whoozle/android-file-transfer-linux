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

#include <usb/Interface.h>
#include <usb/Directory.h>
#include <mtp/log.h>

namespace mtp { namespace usb
{
	Endpoint::Endpoint(const std::string &path): _maxPacketSize(0)
	{
		_addr				= Directory::ReadInt(path + "/bEndpointAddress");

		std::string	type	= Directory::ReadString(path + "/type");
		if (type == "Bulk")
			_type = EndpointType::Bulk;
		else if (type == "Control")
			_type = EndpointType::Control;
		else if (type == "Interrupt")
			_type = EndpointType::Interrupt;
		else if (type == "Isoc")
			_type = EndpointType::Isochronous;
		else
			throw std::runtime_error("invalid endpoint type " + type);

		std::string	dir		= Directory::ReadString(path + "/direction");
		if (dir == "out")
			_direction = EndpointDirection::Out;
		else if (dir == "in")
			_direction = EndpointDirection::In;
		else if (dir == "both")
			_direction = EndpointDirection::Both;
		else
			throw std::runtime_error("invalid endpoint direction " + dir);

		_maxPacketSize		= Directory::ReadInt(path + "/wMaxPacketSize");
	}

	EndpointPtr Endpoint::TryOpen(const std::string &path)
	{
		try
		{ return std::make_shared<Endpoint>(path); }
		catch(const std::exception &ex)
		{ error("failed adding endpoint: ", ex.what()); }
		return nullptr;
	}

}}

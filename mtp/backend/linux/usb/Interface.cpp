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
#include <usb/Interface.h>
#include <usb/Directory.h>

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
		else
			throw std::runtime_error("invalid endpoint direction " + dir);

		_maxPacketSize		= Directory::ReadInt(path + "/wMaxPacketSize");
	}

	Interface::Interface(int index, const std::string &path): _path(path)
	{
		_class		= Directory::ReadInt(path + "/bInterfaceClass");
		_subclass	= Directory::ReadInt(path + "/bInterfaceSubClass");
		_index		= Directory::ReadInt(path + "/bInterfaceNumber");
		try
		{ _name		= Directory::ReadString(path + "/interface"); } catch(const std::exception &ex)
		{ }

		Directory dir(path);
		while(true)
		{
			std::string entry = dir.Read();
			if (entry.empty())
				break;

			if (entry.compare(0, 3, "ep_") == 0)
			{
				try
				{ _endpoints.push_back(std::make_shared<Endpoint>(path + "/" + entry)); }
				catch(const std::exception &ex)
				{ fprintf(stderr, "failed adding endpoint: %s\n", ex.what()); }
			}
		}
	}

}}

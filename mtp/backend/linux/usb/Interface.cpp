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
#include <usb/Endpoint.h>

namespace mtp { namespace usb
{

	Interface::Interface(int index, const std::string &path): _path(path)
	{
		_class		= Directory::ReadInt(path + "/bInterfaceClass");
		_subclass	= Directory::ReadInt(path + "/bInterfaceSubClass");
		_index		= Directory::ReadInt(path + "/bInterfaceNumber");

		Directory dir(path);
		while(true)
		{
			std::string entry = dir.Read();
			if (entry.empty())
				break;

			if (entry.compare(0, 3, "ep_") == 0)
			{
				EndpointPtr ep = Endpoint::TryOpen(path + "/" + entry);
				if (ep)
					_endpoints.push_back(ep);
			}
		}
	}

}}

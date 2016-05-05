/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2016  Vladimir Menshakov

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

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

#ifndef USB_CONTEXT_H
#define USB_CONTEXT_H

#include <mtp/types.h>
#include <usb/DeviceDescriptor.h>
#include <vector>

namespace mtp { namespace usb
{

	class Context : Noncopyable
	{
	public:
		typedef std::vector<DeviceDescriptorPtr> Devices;

	private:
		Devices					_devices;

	public:
		Context();
		~Context();

		void Wait();

		const Devices & GetDevices() const
		{ return _devices; }
	};
	DECLARE_PTR(Context);

}}

#endif


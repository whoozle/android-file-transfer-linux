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

#ifndef AFTL_MTP_BACKEND_DARWIN_USB_CONTEXT_H
#define AFTL_MTP_BACKEND_DARWIN_USB_CONTEXT_H

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


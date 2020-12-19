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

#ifndef AFTL_MTP_PTP_DEVICE_H
#define AFTL_MTP_PTP_DEVICE_H

#include <mtp/usb/BulkPipe.h>
#include <mtp/ptp/PipePacketer.h>
#include <mtp/ptp/Session.h>
#include <usb/DeviceDescriptor.h>

namespace mtp
{
	class Device //! Generic MTP Device class representing physical device, creates \ref Session
	{
		PipePacketer	_packeter;

	private:
		static u8 GetInterfaceStringIndex(usb::DeviceDescriptorPtr desc, u8 number);

	public:
		Device(usb::BulkPipePtr pipe);

		msg::DeviceInfo GetInfo();

		///This could be very expensive, it calls GetDeviceInfo
		bool Matches(const std::string & filter);

		SessionPtr OpenSession(u32 sessionId, int timeout = Session::DefaultTimeout);

		static DevicePtr Open(usb::ContextPtr context, usb::DeviceDescriptorPtr desc, bool claimInterface = true, bool resetDevice = false); //fixme: add flags here
		static DevicePtr FindFirst(usb::ContextPtr context, const std::string & filter = std::string(), bool claimInterface = true, bool resetDevice = false);
		static DevicePtr FindFirst(const std::string & filter = std::string(), bool claimInterface = true, bool resetDevice = false);
	};
}

#endif

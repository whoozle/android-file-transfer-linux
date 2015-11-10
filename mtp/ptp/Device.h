/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015  Vladimir Menshakov

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

#ifndef AFT_PTP_DEVICE
#define AFT_PTP_DEVICE

#include <mtp/usb/BulkPipe.h>
#include <mtp/ptp/PipePacketer.h>
#include <mtp/ptp/Session.h>
#include <usb/DeviceDescriptor.h>

namespace mtp
{
	class Device
	{
		PipePacketer	_packeter;

	private:
		static int GetInterfaceStringIndex(usb::DeviceDescriptorPtr desc, u8 number);

	public:
		Device(usb::BulkPipePtr pipe);

		SessionPtr OpenSession(u32 sessionId, int timeout = 3000);

		static DevicePtr Find(); //fixme: returns first device only
	};
}

#endif

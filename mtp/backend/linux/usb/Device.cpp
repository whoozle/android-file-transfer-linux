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
#include <usb/Device.h>
#include <usb/Exception.h>
#include <mtp/usb/TimeoutException.h>
#include <mtp/ByteArray.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <sys/time.h>

#include "linux/usbdevice_fs.h"

#define IOCTL(...) do { int r = ioctl(__VA_ARGS__); if (r < 0) throw Exception("ioctl(" #__VA_ARGS__ ")"); } while(false)

namespace mtp { namespace usb
{
	Device::InterfaceToken::InterfaceToken(int fd, unsigned interfaceNumber): _fd(fd), _interfaceNumber(interfaceNumber)
	{
		IOCTL(_fd, USBDEVFS_CLAIMINTERFACE, &interfaceNumber);
	}

	Device::InterfaceToken::~InterfaceToken()
	{
		ioctl(_fd, USBDEVFS_RELEASEINTERFACE, &_interfaceNumber);
	}

	Device::Device(int fd): _fd(fd)
	{
		IOCTL(_fd, USBDEVFS_GET_CAPABILITIES, &_capabilities);
		int r = lockf(_fd, F_TLOCK, 0);
		if (r == -1)
			throw Exception("device is used by another process");
	}

	Device::~Device()
	{
		lockf(_fd, F_ULOCK, 0);
		close(_fd);
	}

	int Device::GetConfiguration() const
	{
		return 0;
	}

	void Device::SetConfiguration(int idx)
	{
		fprintf(stderr, "SetConfiguration(%d): not implemented", idx);
	}

	void * Device::Reap(int timeout)
	{
		timeval started = {};
		if (gettimeofday(&started, NULL) == -1)
			throw usb::Exception("gettimeofday");
		pollfd fd = {};
		fd.fd		= _fd;
		fd.events	= POLLOUT;
		int r = poll(&fd, 1, timeout);

		timeval now = {};
		if (gettimeofday(&now, NULL) == -1)
			throw usb::Exception("gettimeofday");

		if (r < 0)
			throw Exception("poll");
		if (r == 0)
		{
			int ms = (now.tv_sec - started.tv_sec) * 1000 + (now.tv_usec - started.tv_usec) / 1000;
			fprintf(stderr, "%d ms since the last poll call\n", ms);
			throw TimeoutException("timeout reaping usb urb");
		}

		usbdevfs_urb *urb;
		r = ioctl(_fd, USBDEVFS_REAPURBNDELAY, &urb);
		if (r == 0)
			return urb;
		else
			throw Exception("ioctl");
	}

	void Device::ReapSingleUrb(void *urb, int timeout)
	{
		void * reapUrb = Reap(timeout);
		if (urb != reapUrb)
		{
			fprintf(stderr, "reaping unknown urb, usb bus conflict: %p %p\n", urb, reapUrb);
			std::terminate();
		}
	}


	void Device::WriteBulk(const EndpointPtr & ep, const IObjectInputStreamPtr &inputStream, int timeout)
	{
		size_t transferSize = ep->GetMaxPacketSize() * 1024;
		ByteArray data(transferSize);

		size_t r;
		bool continuation = false;
		do
		{
			r = inputStream->Read(data.data(), data.size());
			//HexDump("write", ByteArray(data.data(), data.data() + r));
			usbdevfs_urb urb = {};
			urb.type = USBDEVFS_URB_TYPE_BULK;
			urb.endpoint = ep->GetAddress();
			urb.buffer = const_cast<u8 *>(data.data());
			urb.buffer_length = r;
			if (continuation)
				urb.flags |= USBDEVFS_URB_BULK_CONTINUATION;
			else
				continuation = true;
			IOCTL(_fd, USBDEVFS_SUBMITURB, &urb);
			try
			{
				ReapSingleUrb(&urb, timeout);
			}
			catch(const std::exception &ex)
			{
				int r = ioctl(_fd, USBDEVFS_DISCARDURB, &urb);
				if (r != 0)
					std::terminate();
				fprintf(stderr, "exception %s: discard = %d\n", ex.what(), r);
				throw;
			}
		}
		while(r == transferSize);
	}

	void Device::ReadBulk(const EndpointPtr & ep, const IObjectOutputStreamPtr &outputStream, int timeout)
	{
		ByteArray data(ep->GetMaxPacketSize() * 1024);
		usbdevfs_urb urb = {};
		bool continuation = false;
		do
		{
			urb.type = USBDEVFS_URB_TYPE_BULK;
			urb.endpoint = ep->GetAddress();
			urb.buffer = data.data();
			urb.buffer_length = data.size();
			if (continuation)
				urb.flags |= USBDEVFS_URB_BULK_CONTINUATION;
			else
				continuation = true;
			IOCTL(_fd, USBDEVFS_SUBMITURB, &urb);

			try
			{
				ReapSingleUrb(&urb, timeout);
			}
			catch(const std::exception &ex)
			{
				int r = ioctl(_fd, USBDEVFS_DISCARDURB, &urb);
				if (r != 0)
					std::terminate();
				fprintf(stderr, "exception %s: discard = %d\n", ex.what(), r);
				throw;
			}
			//HexDump("read", ByteArray(data.data(), data.data() + urb.actual_length));
			outputStream->Write(data.data(), urb.actual_length);
		}
		while(urb.actual_length == (int)data.size());
	}

}}
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
#include <mtp/ByteArray.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>

#include "linux/usbdevice_fs.h"

#define IOCTL(...) do { int r = ioctl(__VA_ARGS__); if (r < 0) throw Exception(#__VA_ARGS__); } while(false)

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
	}

	Device::~Device()
	{
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
		timespec started = {};
		if (clock_gettime(CLOCK_MONOTONIC, &started) == -1)
			throw Exception("clock_gettime");

		while(true)
		{
			usbdevfs_urb *urb;
			int r = ioctl(_fd, USBDEVFS_REAPURBNDELAY, &urb);
			if (r == 0)
				return urb;

			if (errno == EAGAIN)
			{
				timespec now = {};
				if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
					throw Exception("clock_gettime");
				int delta = (now.tv_sec - started.tv_sec) * 1000 + (now.tv_nsec - started.tv_nsec) / 1000000;
				if (timeout > 0 && delta >= timeout)
					throw std::runtime_error("timeout reaping usb urb");
				usleep(100); //0.1ms
				continue;
			}
			else
				throw Exception("ioctl");
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
			urb.usercontext = this;
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
				usbdevfs_urb *reapedUrb = static_cast<usbdevfs_urb *>(Reap(timeout));
				printf("write %p %p\n", &urb, reapedUrb);
			}
			catch(const std::exception &ex)
			{
				int r = ioctl(_fd, USBDEVFS_DISCARDURB, &urb);
				printf("exception %s: discard = %d\n", ex.what(), r);
				throw;
			}
		}
		while(r == transferSize);
	}

	void Device::ReadBulk(const EndpointPtr & ep, const IObjectOutputStreamPtr &outputStream, int timeout)
	{
		ByteArray data(ep->GetMaxPacketSize() * 1024);
		usbdevfs_urb urb = {};
		urb.usercontext = this;
		urb.type = USBDEVFS_URB_TYPE_BULK;
		urb.endpoint = ep->GetAddress();
		urb.buffer = data.data();
		urb.buffer_length = data.size();
		IOCTL(_fd, USBDEVFS_SUBMITURB, &urb);

		try
		{
			usbdevfs_urb *reapedUrb = static_cast<usbdevfs_urb *>(Reap(timeout));
			printf("read %p %p\n", &urb, reapedUrb);
		}
		catch(const std::exception &ex)
		{
			int r = ioctl(_fd, USBDEVFS_DISCARDURB, &urb);
			printf("exception %s: discard = %d\n", ex.what(), r);
			throw;
		}
		//HexDump("read", ByteArray(data.data(), data.data() + urb.actual_length));
		outputStream->Write(data.data(), urb.actual_length);
	}

}}
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

#include <usb/BufferAllocator.h>
#include <usb/Device.h>
#include <Exception.h>
#include <mtp/usb/TimeoutException.h>
#include <mtp/usb/DeviceBusyException.h>
#include <mtp/usb/DeviceNotFoundException.h>
#include <mtp/ByteArray.h>
#include <mtp/log.h>

#include <sys/ioctl.h>
#include <sys/time.h>
#include <poll.h>
#include <signal.h>

#include "linux/usbdevice_fs.h"

#define IOCTL(FD, ...) do \
{ \
	int r = ioctl(FD, __VA_ARGS__); \
	if (r < 0) \
	{ \
		if (errno == EBUSY) \
			throw DeviceBusyException(FD); \
		else if (errno == ENODEV) \
			throw DeviceNotFoundException(); \
		else \
			throw posix::Exception("ioctl(" #__VA_ARGS__ ")"); \
	} \
} while(false)

namespace mtp { namespace usb
{

	InterfaceToken::InterfaceToken(int fd, unsigned interfaceNumber): _fd(fd), _interfaceNumber(interfaceNumber)
	{
		usbdevfs_disconnect_claim claim = {};
		claim.interface = interfaceNumber;
		IOCTL(_fd, USBDEVFS_DISCONNECT_CLAIM, &claim);
	}

	InterfaceToken::~InterfaceToken()
	{
		ioctl(_fd, USBDEVFS_RELEASEINTERFACE, &_interfaceNumber);
	}

#define PRINT_CAP(CAP, NAME) \
	if (capabilities & (CAP)) \
	{ \
		debug(NAME " "); \
		capabilities &= ~(CAP); \
	}


	Device::Device(int fd, const EndpointPtr &controlEp, u8 configuration):
		_fd(fd), _capabilities(0), _controlEp(controlEp), _configuration(configuration)
	{
		try { IOCTL(_fd.Get(), USBDEVFS_GET_CAPABILITIES, &_capabilities); }
		catch(const std::exception &ex)
		{ error("get usbfs capabilities failed: ", ex.what()); }

		debug("capabilities = 0x", hex(_capabilities, 8));
		bool mmap = _capabilities & USBDEVFS_CAP_MMAP;
		//disable mmap allocation for now, see https://github.com/whoozle/android-file-transfer-linux/issues/194
#if 1
		mmap = false;
#endif
		_bufferAllocator = std::make_shared<BufferAllocator>(mmap? fd: -1);

		if (_capabilities)
		{
			u32 capabilities = _capabilities;
			PRINT_CAP(USBDEVFS_CAP_ZERO_PACKET, "<zero-packet>");
			PRINT_CAP(USBDEVFS_CAP_BULK_CONTINUATION, "<bulk-continuation>");
			PRINT_CAP(USBDEVFS_CAP_NO_PACKET_SIZE_LIM, "<no-packet-size-limit>");
			PRINT_CAP(USBDEVFS_CAP_BULK_SCATTER_GATHER, "<bulk-scatter-gather>");
			PRINT_CAP(USBDEVFS_CAP_REAP_AFTER_DISCONNECT, "<reap-after-disconnect>");
			PRINT_CAP(USBDEVFS_CAP_MMAP, "<mmap>");
			PRINT_CAP(USBDEVFS_CAP_DROP_PRIVILEGES, "<drop-privileges>");
			PRINT_CAP(USBDEVFS_CAP_CONNINFO_EX, "<conninfo-ex>");
			PRINT_CAP(USBDEVFS_CAP_SUSPEND, "<suspend>");
			if (capabilities)
				debug("<unknown capability 0x", hex(capabilities, 2), ">");
		}
		else
			debug("[none]\n");
	}

	Device::~Device()
	{ }

	void Device::Reset()
	{
		debug("resetting device...");
		try
		{
			IOCTL(_fd.Get(), USBDEVFS_RESET);
			SetConfiguration(_configuration);
		}
		catch(const std::exception &ex)
		{ error("resetting device failed: ", ex.what()); }
	}

	int Device::GetConfiguration() const
	{
		return _configuration;
	}

	void Device::SetConfiguration(int idx)
	{
		debug("SetConfiguration(", idx, ")");
		IOCTL(_fd.Get(), USBDEVFS_SETCONFIGURATION, &idx);
		_configuration = idx;
	}

	struct Device::Urb : usbdevfs_urb, Noncopyable
	{
		static const int 		MaxBufferSize = 4096;
		BufferAllocator &		Allocator;
		int						Fd;
		int						PacketSize;
		Buffer					DataBuffer;

		Urb(BufferAllocator & allocator, int fd, u8 urbType, const EndpointPtr & ep):
			usbdevfs_urb(),
			Allocator(allocator),
			Fd(fd),
			PacketSize(ep->GetMaxPacketSize()),
			DataBuffer(Allocator.Allocate(std::max(PacketSize, MaxBufferSize / PacketSize * PacketSize)))
		{
			type			= urbType;
			endpoint		= ep->GetAddress();
			buffer			= DataBuffer.GetData();
			buffer_length	= DataBuffer.GetSize();
		}

		usbdevfs_urb *GetKernelUrb()
		{ return static_cast<usbdevfs_urb *>(this); }

		~Urb()
		{ Allocator.Free(DataBuffer); }

		size_t GetTransferSize() const
		{ return DataBuffer.GetSize(); }

		void Submit()
		{
			IOCTL(Fd, USBDEVFS_SUBMITURB, GetKernelUrb());
		}

		void Discard()
		{
			int r = ioctl(Fd, USBDEVFS_DISCARDURB, GetKernelUrb());
			if (r != 0)
			{
				perror("ioctl(USBDEVFS_DISCARDURB)");
			}
		}

		size_t Send(const IObjectInputStreamPtr &inputStream, size_t size)
		{
			if (size > DataBuffer.GetSize())
				throw std::logic_error("invalid size passed to Send");
			auto data = DataBuffer.GetData();
			size_t r = inputStream->Read(data, size);
			//HexDump("write", ByteArray(data, data + r), true);
			buffer_length = r;
			return r;
		}

		size_t Send(const ByteArray &inputData)
		{
			size_t r = std::min(DataBuffer.GetSize(), inputData.size());
			std::copy(inputData.data(), inputData.data() + r, DataBuffer.GetData());
			buffer_length = r;
			return r;
		}

		size_t Recv(const IObjectOutputStreamPtr &outputStream)
		{
			auto data = DataBuffer.GetData();
			//HexDump("read", ByteArray(data, data + actual_length), true);
			return outputStream->Write(data, actual_length);
		}

		template<unsigned Flag>
		void SetFlag(bool value)
		{
			if (value)
				flags |= Flag;
			else
				flags &= ~Flag;
		}

		void SetContinuationFlag(bool continuation)
		{ SetFlag<USBDEVFS_URB_BULK_CONTINUATION>(continuation); }

		void SetZeroPacketFlag(bool zero)
		{ SetFlag<USBDEVFS_URB_ZERO_PACKET>(zero); }
	};

	void * Device::Reap(int timeout)
	{
		auto urb = AsyncReap(); //attempt to pick up old urbs
		if (urb)
			return urb;

		timeval started = {};
		if (gettimeofday(&started, NULL) == -1)
			throw posix::Exception("gettimeofday");

		pollfd fd = {};
		fd.fd		= _fd.Get();
		fd.events	= POLLOUT | POLLWRNORM;
		int r = poll(&fd, 1, timeout);

		if (r < 0)
			throw posix::Exception("poll");

		timeval now = {};
		if (gettimeofday(&now, NULL) == -1)
			throw posix::Exception("gettimeofday");

		if (r == 0 && timeout > 0)
		{
			int ms = (now.tv_sec - started.tv_sec) * 1000 + (now.tv_usec - started.tv_usec) / 1000;
			error(ms, " ms since the last poll call");
		}
		urb = AsyncReap();
		if (urb)
			return urb;
		else
			throw TimeoutException("timeout reaping usb urb");
	}

	void * Device::AsyncReap()
	{
		usbdevfs_urb *urb;
		int r = ioctl(_fd.Get(), USBDEVFS_REAPURBNDELAY, &urb);
		if (r == 0)
			return urb;
		else if (errno == EAGAIN)
			return nullptr;
		else
			throw posix::Exception("ioctl(USBDEVFS_REAPURBNDELAY)");
	}

	void Device::ClearHalt(const EndpointPtr & ep)
	{
		try
		{ unsigned index = ep->GetAddress(); IOCTL(_fd.Get(), USBDEVFS_CLEAR_HALT, &index); }
		catch(const std::exception &ex)
		{ error("clearing halt status for ep ", hex(ep->GetAddress(), 2), ": ", ex.what()); }
	}

	void Device::Submit(Urb *urb, int timeout)
	{
		urb->Submit();
		try
		{
			while(true)
			{
				usbdevfs_urb * completedKernelUrb = static_cast<usbdevfs_urb *>(Reap(timeout));
				if (urb->GetKernelUrb() != completedKernelUrb)
				{
					error("got unknown urb: ", completedKernelUrb, " of size ", completedKernelUrb->buffer_length);
					continue;
				}
				else
					break;
			}
		}
		catch(const TimeoutException &ex)
		{
			urb->Discard();
			throw;
		}
		catch(const std::exception &ex)
		{
			error("error while submitting urb: ", ex.what());
			urb->Discard();
			throw;
		}
	}

	void Device::WriteBulk(const EndpointPtr & ep, const IObjectInputStreamPtr &inputStream, int timeout)
	{
		Urb urb(*_bufferAllocator, _fd.Get(), USBDEVFS_URB_TYPE_BULK, ep);
		size_t transferSize = urb.GetTransferSize();

		size_t r;
		bool continuation = false;
		do
		{
			r = urb.Send(inputStream, transferSize);

			if (_capabilities & USBDEVFS_CAP_ZERO_PACKET)
				urb.SetZeroPacketFlag(r != transferSize);

			if (_capabilities & USBDEVFS_CAP_BULK_CONTINUATION)
			{
				urb.SetContinuationFlag(continuation);
				continuation = true;
			}
			Submit(&urb, timeout);
		}
		while(r == transferSize);
	}

	void Device::ReadBulk(const EndpointPtr & ep, const IObjectOutputStreamPtr &outputStream, int timeout)
	{
		Urb urb(*_bufferAllocator, _fd.Get(), USBDEVFS_URB_TYPE_BULK, ep);
		size_t transferSize = urb.GetTransferSize();

		size_t r;
		bool continuation = false;
		do
		{
			if (_capabilities & USBDEVFS_CAP_BULK_CONTINUATION)
			{
				urb.SetContinuationFlag(continuation);
				continuation = true;
			}
			Submit(&urb, timeout);

			r = urb.Recv(outputStream);
		}
		while(r == transferSize);
	}

	u8 Device::TransactionType(const EndpointPtr &ep)
	{
		EndpointType type = ep->GetType();
		switch(type)
		{
		case EndpointType::Control:
			return USBDEVFS_URB_TYPE_CONTROL;
		case EndpointType::Isochronous:
			return USBDEVFS_URB_TYPE_ISO;
		case EndpointType::Bulk:
			return USBDEVFS_URB_TYPE_BULK;
		case EndpointType::Interrupt:
			return USBDEVFS_URB_TYPE_INTERRUPT;
		default:
			throw std::runtime_error("invalid endpoint type");
		}
	}

	void Device::ReadControl(u8 type, u8 req, u16 value, u16 index, ByteArray &data, int timeout)
	{
		debug("read control ", hex(type, 2), " ", hex(req, 2), " ", hex(value, 4), " ", hex(index, 4));
		usbdevfs_ctrltransfer ctrl = { };
		ctrl.bRequestType = type;
		ctrl.bRequest = req;
		ctrl.wValue = value;
		ctrl.wIndex = index;
		ctrl.wLength = data.size();
		ctrl.data = const_cast<u8 *>(data.data());
		ctrl.timeout = timeout;

		int fd = _fd.Get();

		int r = ioctl(fd, USBDEVFS_CONTROL, &ctrl);
		if (r >= 0)
			data.resize(r);
		else if (errno == EAGAIN)
			throw TimeoutException("timeout sending control transfer");
		else
			throw posix::Exception("ioctl");
	}

	void Device::WriteControl(u8 type, u8 req, u16 value, u16 index, const ByteArray &data, int timeout)
	{
		debug("write control ", hex(type, 2), " ", hex(req, 2), " ", hex(value, 4), " ", hex(index, 4));
		usbdevfs_ctrltransfer ctrl = { };
		ctrl.bRequestType = type;
		ctrl.bRequest = req;
		ctrl.wValue = value;
		ctrl.wIndex = index;
		ctrl.wLength = data.size();
		ctrl.data = const_cast<u8 *>(data.data());
		ctrl.timeout = timeout;

		int fd = _fd.Get();

		int r = ioctl(fd, USBDEVFS_CONTROL, &ctrl);
		if (r >= 0)
			return;
		else if (errno == EAGAIN)
			throw TimeoutException("timeout sending control transfer");
		else
			throw posix::Exception("ioctl");
	}

}}

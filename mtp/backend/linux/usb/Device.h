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

#ifndef AFTL_MTP_BACKEND_LINUX_USB_DEVICE_H
#define AFTL_MTP_BACKEND_LINUX_USB_DEVICE_H

#include <mtp/types.h>
#include <mtp/ByteArray.h>
#include <usb/Interface.h>
#include <mtp/ptp/IObjectStream.h>
#include <mtp/Token.h>
#include <FileHandler.h>
#include <map>
#include <queue>
#include <functional>

namespace mtp { namespace usb
{
	class Context;
	DECLARE_PTR(Context);
	class BufferAllocator;
	DECLARE_PTR(BufferAllocator);

	class InterfaceToken : public IToken
	{
		int			_fd;
		unsigned	_interfaceNumber;
	public:
		InterfaceToken(int fd, unsigned interfaceNumber);
		~InterfaceToken();
	};
	DECLARE_PTR(InterfaceToken);

	class Device : Noncopyable
	{
	private:
		posix::FileHandler			_fd;
		u32							_capabilities;
		EndpointPtr					_controlEp;
		u8							_configuration;
		BufferAllocatorPtr			_bufferAllocator;

		struct Urb;
		//DECLARE_PTR(Urb);
		std::queue<std::function<void ()>>	_controls;

	public:
		Device(int fd, const EndpointPtr &controlEp, u8 configuration);
		~Device();

		InterfaceTokenPtr ClaimInterface(const InterfacePtr & interface)
		{ return std::make_shared<InterfaceToken>(_fd.Get(), interface->GetIndex()); }

		void Reset();
		int GetConfiguration() const;
		void SetConfiguration(int idx);

		void ClearHalt(const EndpointPtr & ep);
		void WriteBulk(const EndpointPtr & ep, const IObjectInputStreamPtr &inputStream, int timeout);
		void ReadBulk(const EndpointPtr & ep, const IObjectOutputStreamPtr &outputStream, int timeout);

		void WriteControl(u8 type, u8 req, u16 value, u16 index, const ByteArray &data, int timeout);
		void ReadControl(u8 type, u8 req, u16 value, u16 index, ByteArray &data, int timeout); //result buffer must be allocated

	private:
		static u8 TransactionType(const EndpointPtr &ep);
		void * Reap(int timeout);
		void * AsyncReap();
		void Submit(Urb *urb, int timeout);
	};
	DECLARE_PTR(Device);
}}

#endif	/* DEVICE_H */


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

#ifndef DEVICE_H
#define	DEVICE_H

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
		BufferAllocatorPtr			_bufferAllocator;

		struct Urb;
		//DECLARE_PTR(Urb);
		std::queue<std::function<void ()>>	_controls;

	public:
		Device(int fd, const EndpointPtr &controlEp);
		~Device();

		InterfaceTokenPtr ClaimInterface(const InterfacePtr & interface)
		{ return std::make_shared<InterfaceToken>(_fd.Get(), interface->GetIndex()); }

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
		void Submit(Urb *urb, int timeout);
	};
	DECLARE_PTR(Device);
}}

#endif	/* DEVICE_H */


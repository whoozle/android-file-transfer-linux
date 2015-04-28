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
#ifndef DEVICE_H
#define	DEVICE_H

#include <mtp/types.h>
#include <usb/Interface.h>
#include <mtp/ptp/IObjectStream.h>
#include <map>

namespace mtp { namespace usb
{
	class Context;
	DECLARE_PTR(Context);

	class FileHandler : Noncopyable
	{
		int _fd;

	public:
		FileHandler(int fd): _fd(fd) { }
		~FileHandler();

		int Get() const
		{ return _fd; }
	};

	class Device : Noncopyable
	{
	private:
		FileHandler					_fd;
		u32							_capabilities;

		struct Urb;
		DECLARE_PTR(Urb);
		std::map<void *, UrbPtr>	_urbs;

	public:
		Device(int fd);
		~Device();

		class InterfaceToken : Noncopyable
		{
			int			_fd;
			unsigned	_interfaceNumber;
		public:
			InterfaceToken(int fd, unsigned interfaceNumber);
			~InterfaceToken();
		};
		DECLARE_PTR(InterfaceToken);

		InterfaceTokenPtr ClaimInterface(int interfaceNumber)
		{ return std::make_shared<InterfaceToken>(_fd.Get(), interfaceNumber); }

		int GetConfiguration() const;
		void SetConfiguration(int idx);

		void WriteBulk(const EndpointPtr & ep, const IObjectInputStreamPtr &inputStream, int timeout);
		void ReadBulk(const EndpointPtr & ep, const IObjectOutputStreamPtr &outputStream, int timeout);

	private:
		void * Reap(int timeout);
		void Submit(const UrbPtr &urb, int timeout);
	};
	DECLARE_PTR(Device);
}}

#endif	/* DEVICE_H */


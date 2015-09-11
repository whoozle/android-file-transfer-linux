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
#include <usb/Context.h>
#include <usb/call.h>
#include <mtp/ByteArray.h>

namespace mtp { namespace usb
{

	Device::Device(ContextPtr context): _context(context)
	{ }

	Device::~Device()
	{ }

	int Device::GetConfiguration() const
	{ return 0; }

	void Device::SetConfiguration(int idx)
	{ }

	Device::InterfaceToken::InterfaceToken(int index): _index(index)
	{ }

	Device::InterfaceToken::~InterfaceToken()
	{  }

	void Device::WriteBulk(const EndpointPtr & ep, const IObjectInputStreamPtr &inputStream, int timeout)
	{ }

	void Device::ReadBulk(const EndpointPtr & ep, const IObjectOutputStreamPtr &outputStream, int timeout)
	{ }

	void Device::WriteControl(u8 type, u8 req, u16 value, u16 index, const ByteArray &data, bool interruptCurrentTransaction, int timeout)
	{
	}

	std::string Device::GetString(int idx) const
	{
		return std::string();
	}

}}
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
#ifndef IOBJECTSTREAM_H
#define	IOBJECTSTREAM_H

#include <mtp/types.h>

namespace mtp
{

	struct IObjectInputStream
	{
		virtual ~IObjectInputStream() { }
		virtual u64 GetSize() const = 0;
		virtual size_t Read(u8 *data, size_t size) = 0;
	};
	DECLARE_PTR(IObjectInputStream);

	struct IObjectOutputStream
	{
		virtual ~IObjectOutputStream() { }
		virtual size_t Write(const u8 *data, size_t size) = 0;
	};
	DECLARE_PTR(IObjectOutputStream);

}

#endif	/* IOBJECTSTREAM_H */

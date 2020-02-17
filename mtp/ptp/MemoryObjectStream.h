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

#ifndef AFT_MTP_PTP_MEMORYOBJECTSTREAM_H
#define AFT_MTP_PTP_MEMORYOBJECTSTREAM_H

#include <mtp/ByteArray.h>
#include <mtp/ptp/IObjectStream.h>

namespace mtp
{
	class MemoryObjectOutputStream final:
		public IObjectOutputStream,
		public CancellableStream
	{
		ByteArrayPtr _data;

	public:
		MemoryObjectOutputStream(): _data(new ByteArray()) { }

		size_t Write(const u8 *data, size_t size) override
		{
			auto & storage = *_data;
			auto offset = storage.size();
			storage.resize(offset + size);
			std::copy(data, data + size, storage.data() + offset);
			return size;
		}

		const ByteArrayPtr & GetData() const
		{ return _data; }
	};
}

#endif

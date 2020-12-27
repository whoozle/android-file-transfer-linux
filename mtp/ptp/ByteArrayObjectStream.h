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

#ifndef AFTL_MTP_PTP_BYTEARRAYOBJECTSTREAM_H
#define AFTL_MTP_PTP_BYTEARRAYOBJECTSTREAM_H

#include <mtp/ptp/IObjectStream.h>
#include <mtp/ByteArray.h>

namespace mtp
{

	class ByteArrayObjectInputStream final: public IObjectInputStream, public CancellableStream //! stream constructed from \ref ByteArray data
	{
		ByteArray	_data;
		size_t		_offset;

	public:
		ByteArrayObjectInputStream(const ByteArray & data) noexcept: _data(data), _offset(0) { }
		ByteArrayObjectInputStream(ByteArray && data) noexcept: _data(data), _offset(0) { }

		const ByteArray &GetData() const
		{ return _data; }

		virtual u64 GetSize() const
		{ return _data.size(); }

		virtual size_t Read(u8 *data, size_t size)
		{
			CheckCancelled();
			size_t n = std::min(size, _data.size() - _offset);
			std::copy(_data.data() + _offset, _data.data() + _offset + n, data);
			_offset += n;
			return n;
		}
	};
	DECLARE_PTR(ByteArrayObjectInputStream);

	class ByteArrayObjectOutputStream final: public IObjectOutputStream, public CancellableStream //! stream inserting into \ref ByteArray
	{
		ByteArray	_data;

	public:
		ByteArrayObjectOutputStream(): _data() { }

		const ByteArray &GetData() const
		{ return _data; }

		ByteArray &GetData()
		{ return _data; }

		size_t Write(const u8 *data, size_t size) override
		{
			CheckCancelled();
			std::copy(data, data + size, std::back_inserter(_data));
			return size;
		}
	};
	DECLARE_PTR(ByteArrayObjectOutputStream);

	class FixedSizeByteArrayObjectOutputStream final: public IObjectOutputStream, public CancellableStream //! stream writing into fixed size \ref ByteArray
	{
		ByteArray	_data;
		size_t		_offset;

	public:
		FixedSizeByteArrayObjectOutputStream(size_t size): _data(size), _offset(0) { }

		const ByteArray &GetData() const
		{ return _data; }

		size_t Write(const u8 *data, size_t size) override
		{
			CheckCancelled();
			size_t n = std::min(size, _data.size() - _offset);
			std::copy(data, data + n, _data.data() + _offset);
			_offset += n;
			return n;
		}
	};
	DECLARE_PTR(FixedSizeByteArrayObjectOutputStream);

}

#endif	/* BYTEARRAYOBJECTSTREAM_H */

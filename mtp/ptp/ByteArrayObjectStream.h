/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2016  Vladimir Menshakov

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

#ifndef BYTEARRAYOBJECTSTREAM_H
#define	BYTEARRAYOBJECTSTREAM_H

#include <mtp/ptp/IObjectStream.h>
#include <mtp/ByteArray.h>

namespace mtp
{

	class ByteArrayObjectInputStream : public IObjectInputStream, public CancellableStream //! stream constructed from \ref ByteArray data
	{
		ByteArray	_data;
		size_t		_offset;

	public:
		ByteArrayObjectInputStream(const ByteArray & data): _data(data), _offset(0) { }
		ByteArrayObjectInputStream(ByteArray && data): _data(data), _offset(0) { }

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

	class ByteArrayObjectOutputStream : public IObjectOutputStream, public CancellableStream //! stream inserting into \ref ByteArray
	{
		ByteArray	_data;

	public:
		ByteArrayObjectOutputStream(): _data() { }

		const ByteArray &GetData() const
		{ return _data; }

		virtual size_t Write(const u8 *data, size_t size)
		{
			CheckCancelled();
			std::copy(data, data + size, std::back_inserter(_data));
			return size;
		}
	};
	DECLARE_PTR(ByteArrayObjectOutputStream);

	class FixedSizeByteArrayObjectOutputStream : public IObjectOutputStream, public CancellableStream //! stream writing into fixed size \ref ByteArray
	{
		ByteArray	_data;
		size_t		_offset;

	public:
		FixedSizeByteArrayObjectOutputStream(size_t size): _data(size), _offset(0) { }

		const ByteArray &GetData() const
		{ return _data; }

		virtual size_t Write(const u8 *data, size_t size)
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

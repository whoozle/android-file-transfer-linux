/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2018  Vladimir Menshakov

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

#ifndef OUTPUTSTREAM_H
#define	OUTPUTSTREAM_H

#include <mtp/types.h>
#include <mtp/ByteArray.h>

namespace mtp
{

	class OutputStream //! MTP format encoding output stream
	{
		ByteArray	& _data;

	public:
		OutputStream(ByteArray &data): _data(data)
		{ _data.reserve(512); }

		ByteArray & GetData()
		{ return _data; }

		const ByteArray & GetData() const
		{ return _data; }

		void Write8(u8 value)
		{ _data.push_back(value); }

		void Write16(u16 value)
		{ Write8(value); Write8(value >> 8); }

		void Write32(u32 value)
		{ Write16(value); Write16(value >> 16); }

		void Write64(u32 value)
		{ Write32(value); Write32(value >> 16); }

		static size_t Utf8Length(const std::string &value)
		{
			size_t size = 0;
			for(char c: value)
			{
				if ((c & 0x80) == 0 || (c & 0xc0) != 0x80)
					++size;
			}
			return size;
		}

		void WriteString(const std::string &value);

		template<typename ElementType>
		void WriteArray(const std::vector<ElementType> &array)
		{
			Write32(array.size());
			for(const auto &el : array)
				(*this) << el;
		}
	};

	inline OutputStream & operator << (OutputStream &stream, u8 value)
	{ stream.Write8(value); return stream; }

	inline OutputStream & operator << (OutputStream &stream, u16 value)
	{ stream.Write16(value); return stream; }

	inline OutputStream & operator << (OutputStream &stream, u32 value)
	{ stream.Write32(value); return stream; }

	inline OutputStream & operator << (OutputStream &stream, u64 value)
	{ stream.Write64(value); return stream; }

	inline OutputStream & operator << (OutputStream &stream, const std::string &value)
	{ stream.WriteString(value); return stream; }

	template<typename ElementType>
	inline OutputStream & operator << (OutputStream &stream, const std::vector<ElementType> &value)
	{ stream.template WriteArray<ElementType>(value); return stream; }

}

#endif

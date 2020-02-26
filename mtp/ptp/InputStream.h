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

#ifndef AFTL_MTP_PTP_INPUTSTREAM_H
#define AFTL_MTP_PTP_INPUTSTREAM_H

#include <mtp/types.h>
#include <mtp/ByteArray.h>

namespace mtp
{

	class InputStream //! MTP data decoding input stream
	{
		const ByteArray &	_data;
		size_t				_offset;

	public:
		InputStream(const ByteArray & data, size_t offset = 0): _data(data), _offset(offset) { }

		size_t GetOffset() const
		{ return _offset; }

		void Skip(size_t size)
		{ _offset += size; }

		const ByteArray & GetData() const
		{ return _data; }

		bool AtEnd() const
		{ return _offset >= _data.size(); }

		u8 Read8()
		{ return _data.at(_offset++); }

		u16 Read16()
		{
			u8 l = Read8();
			u8 h = Read8();
			return l | ((u16)h << 8);
		}
		u32 Read32()
		{
			u16 l = Read16();
			u16 h = Read16();
			return l | ((u32)h << 16);
		}

		u64 Read64()
		{
			u32 l = Read32();
			u32 h = Read32();
			return l | ((u64)h << 32);
		}

		u64 Read128() {
			u64 l = Read64();
			Skip(8); //fixme: truncated 128 bit value
			return l;
		}

		std::string ReadString()
		{ return ReadString(Read8()); }

		std::string ReadString(unsigned len)
		{
			std::string str;
			str.reserve(2 * len);
			while(len--)
			{
				u16 ch = Read16();
				if (ch == 0)
					continue;
				if (ch <= 0x7f)
					str += (char)ch;
				else if (ch <= 0x7ff)
				{
					str += (char) ((ch >> 6) | 0xc0);
					str += (char) ((ch & 0x3f) | 0x80);
				}
				else //if (ch <= 0xffff)
				{
					str += (char)((ch >> 12) | 0xe0);
					str += (char)(((ch & 0x0fc0) >> 6) | 0x80);
					str += (char)( (ch & 0x003f) | 0x80);
				}
			}
			return str;
		}

		template<typename ElementType>
		std::vector<ElementType> ReadArray()
		{
			std::vector<ElementType> array;
			if (AtEnd())
				return array;

			u32 size = Read32();
			while(size--)
			{
				ElementType el;
				(*this) >> el;
				array.push_back(el);
			}
			return array;
		}
	};

	inline InputStream & operator >> (InputStream &stream, u8 &value)
	{ value = stream.Read8(); return stream; }

	inline InputStream & operator >> (InputStream &stream, u16 &value)
	{ value = stream.Read16(); return stream; }

	inline InputStream & operator >> (InputStream &stream, u32 &value)
	{ value = stream.Read32(); return stream; }

	inline InputStream & operator >> (InputStream &stream, u64 &value)
	{ value = stream.Read64(); return stream; }

	inline InputStream & operator >> (InputStream &stream, s8 &value)
	{ value = stream.Read8(); return stream; }

	inline InputStream & operator >> (InputStream &stream, s16 &value)
	{ value = stream.Read16(); return stream; }

	inline InputStream & operator >> (InputStream &stream, s32 &value)
	{ value = stream.Read32(); return stream; }

	inline InputStream & operator >> (InputStream &stream, s64 &value)
	{ value = stream.Read64(); return stream; }

	inline InputStream & operator >> (InputStream &stream, std::string &value)
	{ value = stream.ReadString(); return stream; }

	template<typename ElementType>
	inline InputStream & operator >> (InputStream &stream, std::vector<ElementType> &value)
	{ value = stream.template ReadArray<ElementType>(); return stream; }

	inline u64 ReadSingleInteger(const ByteArray &data)
	{
		InputStream s(data);
		switch(data.size())
		{
		case 8: return s.Read64();
		case 4: return s.Read32();
		case 2: return s.Read16();
		case 1: return s.Read8();
		default:
			throw std::runtime_error("unexpected length for numeric property");
		}
	}

	inline std::string ReadSingleString(const ByteArray &data)
	{
		InputStream s(data);
		std::string value;
		s >> value;
		return value;
	}

	template<typename ResponseType>
	ResponseType ParseResponse(const ByteArray & data)
	{
		ResponseType response;
		InputStream is(data);
		response.Read(is);
		return response;
	}
}


#endif	/* STREAM_H */

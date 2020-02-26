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

#ifndef AFTL_MTP_LOG_H
#define AFTL_MTP_LOG_H

#include <mtp/ByteArray.h>
#include <mtp/types.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

//I love libstdc++!
inline std::ostream & operator << (std::ostream & stream, unsigned char v)
{
	stream << (mtp::u16)v;
	return stream;
}

namespace mtp
{
	class InputStream;

	inline std::ostream & operator << (std::ostream & stream, unsigned char v)
	{
		stream << (u16)v;
		return stream;
	}

	namespace impl
	{
		template<typename T, bool Hex>
		struct Format
		{
			T			Value;
			unsigned	Width;
			Format(const T & value, unsigned width = 0): Value(value), Width(width) { }

			template<typename Stream>
			Stream & operator >> (Stream & stream) const
			{
				std::ios::fmtflags oldFlags = stream.flags();
				char oldFill = stream.fill();
				if (Hex)
					stream << std::setw(Width) << std::setfill('0') << std::hex << Value;
				else
					stream << std::setw(Width) << std::setfill(' ') << Value;
				stream.flags(oldFlags);
				stream.fill(oldFill);
				return stream;
			}

			std::string ToString() const
			{
				std::stringstream ss;
				(*this) >> ss;
				return ss.str();
			}
		};
	}

	template<typename Stream, typename T, bool H>
	Stream & operator << (Stream &stream, const impl::Format<T, H> &format)
	{
		format >> stream;
		return stream;
	}

	template<typename T>
	impl::Format<T, true> hex(const T & value, unsigned width = 0)
	{ return impl::Format<T, true>(value, width); }

	template<typename T>
	impl::Format<T, false> width(const T & value, unsigned width)
	{ return impl::Format<T, false>(value, width); }

	inline void print()
	{ std::cout << std::endl; }

	template<typename ValueType, typename ... Args>
	void print(const ValueType & value, Args ... args)
	{
		std::cout << value;
		print(args...);
	}

	extern bool g_debug;

	inline void error()
	{ std::cerr << std::endl; }

	template<typename ValueType, typename ... Args>
	void error(const ValueType & value, Args ... args)
	{
		std::cerr << value;
		error(args...);
	}

	template<typename ValueType, typename ... Args>
	void debug(const ValueType & value, Args ... args)
	{
		if (g_debug)
			error(value, args...);
	}

	//! output hex dump to debug channel
	void HexDump(const std::string &prefix, const ByteArray &data, bool force = false);
	void HexDump(std::stringstream & ss, const std::string &prefix, size_t size, InputStream & is);
}

#endif

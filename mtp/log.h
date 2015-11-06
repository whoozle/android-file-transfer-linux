/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015  Vladimir Menshakov

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

#ifndef AFT_LOG_H
#define AFT_LOG_H

#include <mtp/types.h>

#include <iostream>
#include <iomanip>

//I love libstdc++!
inline std::ostream & operator << (std::ostream & stream, unsigned char v)
{
	stream << (mtp::u16)v;;
	return stream;
}

namespace mtp
{

	inline std::ostream & operator << (std::ostream & stream, unsigned char v)
	{
		stream << (u16)v;
		return stream;
	}

	template<typename T>
	struct Hex
	{
		T			Value;
		unsigned	Width;
		Hex(const T & value, unsigned width = 0): Value(value), Width(width) { }

		template<typename Stream>
		Stream & operator >> (Stream & stream) const
		{
			std::ios::fmtflags oldFlags = stream.flags();
			stream << std::setw(Width) << std::setfill('0') << std::hex << Value;
			stream.flags(oldFlags);
			return stream;
		}
	};

	template<typename Stream, typename T>
	Stream & operator << (Stream &stream, const Hex<T> &hex)
	{
		hex >> stream;
		return stream;
	}

	template<typename T>
	Hex<T> hex(const T & value, unsigned width = 0)
	{ return Hex<T>(value, width); }

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
}

#endif

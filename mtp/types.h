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

#ifndef AFTL_MTP_TYPES_H
#define AFTL_MTP_TYPES_H

#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <memory>
#include <mutex>
#include <exception>
#include <stdexcept>
#include <string>

namespace mtp
{
	using scoped_mutex_lock = std::unique_lock<std::mutex>;

	using u8	= uint8_t;
	using u16	= uint16_t;
	using u32	= uint32_t;
	using u64	= uint64_t;

	using s8	= int8_t;
	using s16	= int16_t;
	using s32	= int32_t;
	using s64	= int64_t;

	#define DECLARE_PTR(C) using C##Ptr = std::shared_ptr<C>

	template<typename T>
	inline T RequireNotNull(const T &t, const std::string &expr) {
		if (!t)
			throw std::runtime_error("null check failed " + expr);
		return t;
	}

	struct Noncopyable
	{
		Noncopyable() = default;
		Noncopyable(const Noncopyable&) = delete;
		Noncopyable& operator=(const Noncopyable &) = delete;
	};

	class system_error : public std::exception
	{
		std::string _message;
	public:
		system_error(const std::string &prefix, int err = 0) : _message(prefix + ": " + strerror(err? err: errno)){ }
		virtual const char * what() const throw() { return _message.c_str(); }
	};
}

#define NOT_NULL(x) (mtp::RequireNotNull((x), #x))

#define ASSERT(expr) if (!(expr)) throw std::runtime_error("assertion " #expr " failed")
#define CATCH(WHAT, ...) catch(const std::exception &ex) { LOG(WARNING) << WHAT << ": " << ex.what(); __VA_ARGS__ }

//fixme: c++17 will allow implement it in stream with std::is_enum(c++11) + std::underlying_type(c++17)
#define DECLARE_ENUM(TYPE, BASETYPE) \
	template<typename Stream> \
	Stream &operator << (Stream & stream, TYPE format) \
	{ stream << static_cast<BASETYPE>(format); return stream; } \
 \
	template<typename Stream> \
	Stream &operator >> (Stream & stream, TYPE &format) \
	{ BASETYPE value; stream >> value; format = static_cast<TYPE>(value); return stream; }

#define ENUM_VALUE_DECL(NAME, VALUE) NAME = VALUE ,
#define ENUM_VALUE_TO_STRING(TYPE, NAME, VALUE) case TYPE :: NAME : return #NAME ;
#define ENUM_VALUE_TO_STRING_DEFAULT(TYPE, VALUE, PADDING) default: return hex(static_cast<unsigned>( VALUE ), PADDING ).ToString();


#endif

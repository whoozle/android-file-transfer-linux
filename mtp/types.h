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

#ifndef TYPES_H
#define	TYPES_H

#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <memory>
#include <mutex>
#include <exception>
#include <string>

namespace mtp
{
	typedef std::unique_lock<std::mutex> scoped_mutex_lock;

	typedef uint8_t		u8;
	typedef uint16_t	u16;
	typedef uint32_t	u32;
	typedef uint64_t	u64;

	typedef int8_t		s8;
	typedef int16_t		s16;
	typedef int32_t		s32;
	typedef int64_t		s64;

	#define DECLARE_PTR(C) typedef std::shared_ptr<C> C##Ptr

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

#define NOT_NULL(x) (RequireNotNull((x), #x))

#define ASSERT(expr) if (!(expr)) throw std::runtime_error("assertion " #expr " failed")
#define CATCH(WHAT, ...) catch(const std::exception &ex) { LOG(WARNING) << WHAT << ": " << ex.what(); __VA_ARGS__ }

#define DECLARE_ENUM(TYPE, BASETYPE) \
	template<typename Stream> \
	Stream &operator << (Stream & stream, TYPE format) \
	{ stream << static_cast<BASETYPE>(format); return stream; } \
 \
	template<typename Stream> \
	Stream &operator >> (Stream & stream, TYPE &format) \
	{ BASETYPE value; stream >> value; format = static_cast<TYPE>(value); return stream; }


#endif

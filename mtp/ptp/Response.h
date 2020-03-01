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

#ifndef AFTL_MTP_PTP_RESPONSE_H
#define AFTL_MTP_PTP_RESPONSE_H

#include <mtp/ptp/OperationCode.h>
#include <stdexcept>

namespace mtp
{
	enum struct ContainerType : u16
	{
		Command = 1,
		Data = 2,
		Response = 3,
		Event = 4,
	};
	DECLARE_ENUM(ContainerType, u16);

	enum struct ResponseType : u16
	{
#define ENUM_VALUE(NAME, VALUE) ENUM_VALUE_DECL(NAME, VALUE)
#		include <mtp/ptp/ResponseType.values.h>
#undef ENUM_VALUE
	};
	std::string ToString(ResponseType response);
	DECLARE_ENUM(ResponseType, u16);

	struct Response //! MTP Response class
	{
		static const size_t		Size = 8;

		mtp::ContainerType		ContainerType;
		mtp::ResponseType		ResponseType;
		u32						Transaction;

		Response() { }

		template<typename Stream>
		Response(Stream &stream)
		{ Read(stream); }

		template<typename Stream>
		void Read(Stream &stream)
		{
			stream >> ContainerType;
			stream >> ResponseType;
			stream >> Transaction;
		}
	};

	struct InvalidResponseException : public std::runtime_error //!Invalid MTP Response Exception
	{
		ResponseType Type;
		InvalidResponseException(const std::string &where, ResponseType type);
	};

}

#endif	/* RESPONSE_H */

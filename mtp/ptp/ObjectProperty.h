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

#ifndef OBJECTPROPERTY_H
#define	OBJECTPROPERTY_H

#include <mtp/types.h>
#include <string>

namespace mtp
{
	enum struct DataTypeCode : u16
	{
		Undefined					= 0x0000,

		Int8						= 0x0001,
		Uint8						= 0x0002,
		Int16						= 0x0003,
		Uint16						= 0x0004,
		Int32						= 0x0005,
		Uint32						= 0x0006,
		Int64						= 0x0007,
		Uint64						= 0x0008,
		Int128						= 0x0009,
		Uint128						= 0x000a,

		ArrayInt8					= 0x4001,
		ArrayUint8					= 0x4002,
		ArrayInt16					= 0x4003,
		ArrayUint16					= 0x4004,
		ArrayInt32					= 0x4005,
		ArrayUint32					= 0x4006,
		ArrayInt64					= 0x4007,
		ArrayUint64					= 0x4008,
		ArrayInt128					= 0x4009,
		ArrayUint128				= 0x400a,

		String						= 0xffff
	};
	DECLARE_ENUM(DataTypeCode, u16);

	enum struct ObjectProperty : u16
	{
#define ENUM_VALUE(NAME, VALUE) ENUM_VALUE_DECL(NAME, VALUE)
#		include <mtp/ptp/ObjectProperty.values.h>
#undef ENUM_VALUE
	};
	DECLARE_ENUM(ObjectProperty, u16);

	std::string ToString(ObjectProperty property);

}

#endif	/* OBJECTPROPERTY_H */

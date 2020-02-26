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

#ifndef AFTL_MTP_PTP_OBJECTPROPERTY_H
#define AFTL_MTP_PTP_OBJECTPROPERTY_H

#include <mtp/ptp/DataTypeCode.h>
#include <mtp/ByteArray.h>
#include <mtp/types.h>
#include <string>

namespace mtp
{

	enum struct ObjectProperty : u16
	{
#define ENUM_VALUE(NAME, VALUE) ENUM_VALUE_DECL(NAME, VALUE)
#		include <mtp/ptp/ObjectProperty.values.h>
#undef ENUM_VALUE
	};
	DECLARE_ENUM(ObjectProperty, u16);

	std::string ToString(ObjectProperty property);
	std::string ToString(DataTypeCode type, const ByteArray & value);

}

#endif	/* OBJECTPROPERTY_H */

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

#ifndef AFTL_MTP_PTP_OBJECTID_H
#define AFTL_MTP_PTP_OBJECTID_H

#include <mtp/types.h>

namespace mtp
{

#define DECLARE_ID(NAME, TYPE, MEMBER) \
	struct NAME { \
		TYPE MEMBER; \
		static const TYPE Canary = 0xaaaaaaaau; \
 \
		NAME ( ): MEMBER ( Canary ) { } \
		explicit NAME ( TYPE id ): MEMBER ( id ) { } \
 \
		bool operator == (const NAME &o) const \
		{ return MEMBER == o.MEMBER; } \
		bool operator != (const NAME &o) const \
		{ return !((*this) == o); } \
		bool operator < (const NAME &o) const \
		{ return MEMBER < o.MEMBER; } \
	}; \
	template <typename Stream> Stream & operator >> (Stream &stream, NAME & value) \
	{ stream >> value. MEMBER ; return stream; } \
	template <typename Stream> Stream & operator << (Stream &stream, const NAME &value) \
	{ stream << value. MEMBER ; return stream; } \


	DECLARE_ID(ObjectId, mtp::u32, Id);
	DECLARE_ID(StorageId, mtp::u32, Id);

#undef DECLARE_ID

}

namespace std
{
	template<> struct hash<mtp::ObjectId>
	{ public: size_t operator()(const mtp::ObjectId &id) const { return id.Id; } };
	template<> struct hash<mtp::StorageId>
	{ public: size_t operator()(const mtp::StorageId &id) const { return id.Id; } };
}
#endif



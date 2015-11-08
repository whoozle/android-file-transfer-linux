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

#ifndef OBJECTPROPERTY_H
#define	OBJECTPROPERTY_H

#include <mtp/types.h>

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

	enum struct ObjectProperty : u16
	{
		StorageId					= 0xdc01,
		ObjectFormat				= 0xdc02,
		ProtectionStatus			= 0xdc03,
		ObjectSize					= 0xdc04,
		AssociationType				= 0xdc05,
		AssociationDesc				= 0xdc06,
		ObjectFilename				= 0xdc07,
		DateCreated					= 0xdc08,
		DateModified				= 0xdc09,
		Keywords					= 0xdc0a,
		ParentObject				= 0xdc0b,
		AllowedFolderContents		= 0xdc0c,
		Hidden						= 0xdc0d,
		SystemObject				= 0xdc0e,

		PersistentUniqueObjectId	= 0xdc41,
		SyncId						= 0xdc42,
		Name						= 0xdc44,
		Artist						= 0xdc46,
		DateAuthored				= 0xdc47,
		DateAdded					= 0xdc4e,

		RepresentativeSampleFormat	= 0xdc81,
		RepresentativeSampleData	= 0xdc86,

		DisplayName					= 0xdce0,
		BodyText					= 0xdce1,
		Subject						= 0xdce2,
		Priority					= 0xdce3,

		MediaGUID					= 0xdd72
	};

}

#endif	/* OBJECTPROPERTY_H */

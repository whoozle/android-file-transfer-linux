/*
 * Android File Transfer for Linux: MTP client for android devices
 * Copyright (C) 2015  Vladimir Menshakov

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef OBJECTPROPERTY_H
#define	OBJECTPROPERTY_H

namespace mtp
{

	enum struct ObjectProperty
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

		RepresentativeSampleFormat	= 0xdc81,
		RepresentativeSampleData	= 0xdc86,

		MediaGUID					= 0xdd72
	};

}

#endif	/* OBJECTPROPERTY_H */

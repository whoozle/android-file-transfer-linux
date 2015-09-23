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

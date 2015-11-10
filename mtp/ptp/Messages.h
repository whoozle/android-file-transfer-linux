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

#ifndef MESSAGES_H
#define	MESSAGES_H

#include <mtp/ptp/InputStream.h>
#include <mtp/ptp/OutputStream.h>
#include <mtp/ptp/ObjectFormat.h>
#include <mtp/ptp/ObjectId.h>
#include <mtp/ptp/ObjectProperty.h>
#include <mtp/ptp/OperationCode.h>
#include <algorithm>

namespace mtp { namespace msg
{
	struct DeviceInfo
	{
		u16							StandardVersion;
		u32							VendorExtensionId;
		u16							VendorExtensionVersion;
		std::string					VendorExtensionDesc;
		u16							FunctionalMode;
		std::vector<OperationCode>	OperationsSupported;
		std::vector<u16>			EventsSupported;
		std::vector<u16>			DevicePropertiesSupported;
		std::vector<u16>			CaptureFormats;
		std::vector<u16>			ImageFormats;
		std::string					Manufacturer;
		std::string					Model;
		std::string					DeviceVersion;
		std::string					SerialNumber;

		void Read(InputStream &stream)
		{
			stream >> StandardVersion;
			stream >> VendorExtensionId;
			stream >> VendorExtensionVersion;
			stream >> VendorExtensionDesc;
			stream >> FunctionalMode;
			stream >> OperationsSupported;
			stream >> EventsSupported;
			stream >> DevicePropertiesSupported;
			stream >> CaptureFormats;
			stream >> ImageFormats;
			stream >> Manufacturer;
			stream >> Model;
			stream >> DeviceVersion;
			stream >> SerialNumber;
		}

		bool Supports(OperationCode opcode) const
		{
			auto i = std::find(OperationsSupported.begin(), OperationsSupported.end(), opcode);
			return i != OperationsSupported.end();
		}
	};

	struct ObjectHandles
	{
		std::vector<ObjectId> ObjectHandles;

		void Read(InputStream &stream)
		{
			stream >> ObjectHandles;
		}
	};

	struct StorageIDs
	{
		std::vector<StorageId> StorageIDs;

		void Read(InputStream &stream)
		{
			stream >> StorageIDs;
		}
	};

	struct StorageInfo
	{
		u16			StorageType;
		u16			FilesystemType;
		u16			AccessCapability;
		u64			MaxCapacity;
		u64			FreeSpaceInBytes;
		u32			FreeSpaceInImages;
		std::string	StorageDescription;
		std::string	VolumeLabel;


		void Read(InputStream &stream)
		{
			stream >> StorageType;
			stream >> FilesystemType;
			stream >> AccessCapability;
			stream >> MaxCapacity;
			stream >> FreeSpaceInBytes;
			stream >> FreeSpaceInImages;
			stream >> StorageDescription;
			stream >> VolumeLabel;
		}
	};

	struct ObjectInfo
	{
		mtp::StorageId		StorageId;
		mtp::ObjectFormat	ObjectFormat;
		u16					ProtectionStatus;
		u32					ObjectCompressedSize;
		u16					ThumbFormat;
		u32					ThumbCompressedSize;
		u32					ThumbPixWidth;
		u32					ThumbPixHeight;
		u32					ImagePixWidth;
		u32					ImagePixHeight;
		u32					ImageBitDepth;
		ObjectId			ParentObject;
		mtp::AssociationType AssociationType;
		u32					AssociationDesc;
		u32					SequenceNumber;
		std::string			Filename;
		std::string			CaptureDate;
		std::string			ModificationDate;
		std::string			Keywords;

		ObjectInfo(): StorageId(), ObjectFormat(), ProtectionStatus(), ObjectCompressedSize(),
			ThumbFormat(), ThumbCompressedSize(), ThumbPixWidth(), ThumbPixHeight(),
			ImagePixWidth(), ImagePixHeight(), ImageBitDepth(),
			ParentObject(), AssociationType(), AssociationDesc(),
			SequenceNumber()
		{ }

		void SetSize(u64 size)
		{
			ObjectCompressedSize = (size > MaxObjectSize)? MaxObjectSize: size;
		}

		void Read(InputStream &stream)
		{
			stream >> StorageId;
			stream >> ObjectFormat;
			stream >> ProtectionStatus;
			stream >> ObjectCompressedSize;
			stream >> ThumbFormat;
			stream >> ThumbCompressedSize;
			stream >> ThumbPixWidth;
			stream >> ThumbPixHeight;
			stream >> ImagePixWidth;
			stream >> ImagePixHeight;
			stream >> ImageBitDepth;
			stream >> ParentObject;
			stream >> AssociationType;
			stream >> AssociationDesc;
			stream >> SequenceNumber;
			stream >> Filename;
			stream >> CaptureDate;
			stream >> ModificationDate;
			stream >> Keywords;
		}
//fixme: make me template
		void Write(OutputStream &stream) const
		{
			stream << StorageId;
			stream << ObjectFormat;
			stream << ProtectionStatus;
			stream << ObjectCompressedSize;
			stream << ThumbFormat;
			stream << ThumbCompressedSize;
			stream << ThumbPixWidth;
			stream << ThumbPixHeight;
			stream << ImagePixWidth;
			stream << ImagePixHeight;
			stream << ImageBitDepth;
			stream << ParentObject;
			stream << AssociationType;
			stream << AssociationDesc;
			stream << SequenceNumber;
			stream << Filename;
			stream << CaptureDate;
			stream << ModificationDate;
			stream << Keywords;
		}
	};
	DECLARE_PTR(ObjectInfo);

	struct ObjectPropertiesSupported
	{
		std::vector<ObjectProperty>		ObjectPropertyCodes;

		void Read(InputStream &stream)
		{
			stream >> ObjectPropertyCodes;
		}
	};

}}


#endif	/* MESSAGES_H */

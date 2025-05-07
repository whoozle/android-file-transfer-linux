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

#ifndef AFTL_MTP_PTP_MESSAGES_H
#define AFTL_MTP_PTP_MESSAGES_H

#include <mtp/ptp/DeviceProperty.h>
#include <mtp/ptp/InputStream.h>
#include <mtp/ptp/OutputStream.h>
#include <mtp/ptp/ObjectFormat.h>
#include <mtp/ptp/ObjectId.h>
#include <mtp/ptp/ObjectProperty.h>
#include <mtp/ptp/OperationCode.h>
#include <mtp/ptp/EventCode.h>
#include <string>
#include <vector>

namespace mtp { namespace msg
{
	struct DeviceInfo //! MTP DeviceInfo message
	{
		u16							StandardVersion;
		u32							VendorExtensionId;
		u16							VendorExtensionVersion;
		std::string					VendorExtensionDesc;
		u16							FunctionalMode;
		std::vector<OperationCode>	OperationsSupported;
		std::vector<EventCode>		EventsSupported;
		std::vector<DeviceProperty>	DevicePropertiesSupported;
		std::vector<ObjectFormat>	CaptureFormats;
		std::vector<ObjectFormat>	ImageFormats;
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

		bool Supports(OperationCode opcode) const;

		bool Supports(DeviceProperty property) const;

		bool Supports(EventCode event) const;

		bool Supports(ObjectFormat format) const;

		std::string GetFilesystemFriendlyName() const;

		static bool Matches(const std::string & haystack, const std::string & needle);

		bool Matches(const std::string & filter) const;
	};

	struct ObjectHandles //! MTP ObjectHandles message
	{
		std::vector<ObjectId> ObjectHandles;

		void Read(InputStream &stream)
		{
			stream >> ObjectHandles;
		}

		void Write(OutputStream & stream) const
		{
			stream.WriteArray(ObjectHandles);
		}
	};

	struct StorageIDs //! MTP StorageIDs message
	{
		std::vector<StorageId> StorageIDs;

		void Read(InputStream &stream)
		{
			stream >> StorageIDs;
		}
	};

	struct StorageInfo //! MTP StorageInfo message
	{
		u16			StorageType;
		u16			FilesystemType;
		u16			AccessCapability;
		u64			MaxCapacity;
		u64			FreeSpaceInBytes;
		u32			FreeSpaceInImages;
		std::string	StorageDescription;
		std::string	VolumeLabel;

		std::string GetName() const
		{ return !StorageDescription.empty()? StorageDescription: VolumeLabel; }

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

	struct ObjectInfo //! MTP ObjectInfo message
	{
		mtp::StorageId		StorageId;
		mtp::ObjectFormat	ObjectFormat;
		u16					ProtectionStatus;
		u64					ObjectCompressedSize;
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

		void Read(InputStream &stream)
		{
			stream >> StorageId;
			stream >> ObjectFormat;
			stream >> ProtectionStatus;
			u32 objectCompressedSize;
			stream >> objectCompressedSize;
			ObjectCompressedSize = objectCompressedSize;
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
			u32 objectCompressedSize = (ObjectCompressedSize > MaxObjectSize)? MaxObjectSize: ObjectCompressedSize;
			stream << objectCompressedSize;
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

	struct ObjectPropertiesSupported  //! MTP ObjectPropSupported message
	{
		std::vector<ObjectProperty>		ObjectPropertyCodes;

		bool Supports(ObjectProperty prop) const;

		void Read(InputStream &stream)
		{
			stream >> ObjectPropertyCodes;
		}
	};

	struct SendObjectPropListResponse
	{
		mtp::StorageId		StorageId;
		mtp::ObjectId		ParentObjectId;
		mtp::ObjectId		ObjectId;
		u32					FailedPropertyIndex;

		SendObjectPropListResponse():
			StorageId(), ParentObjectId(), ObjectId(),
			FailedPropertyIndex()
		{ }

		void Read(InputStream &stream)
		{
			stream >> StorageId;
			stream >> ParentObjectId;
			stream >> ObjectId;
			stream >> FailedPropertyIndex;
		}

	};

	struct DevicePropertyDesc
	{
		DeviceProperty 	Property;
		DataTypeCode	Type;
		bool			Writeable;

		DevicePropertyDesc(): Property(), Type(DataTypeCode::Undefined), Writeable()
		{ }

		void Read(InputStream &stream)
		{
			stream >> Property;
			stream >> Type;
			u8 writeable;
			stream >> writeable;
			Writeable = writeable;
		}
	};

	struct NewObjectInfo
	{
		mtp::StorageId		StorageId;
		mtp::ObjectId		ParentObjectId;
		mtp::ObjectId		ObjectId;

		void Read(InputStream & is)
		{
			is >> StorageId;
			is >> ParentObjectId;
			is >> ObjectId;
		}
	};

}}


#endif	/* MESSAGES_H */

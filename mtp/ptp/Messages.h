#ifndef MESSAGES_H
#define	MESSAGES_H

#include <mtp/ptp/Stream.h>

namespace mtp { namespace msg
{
	struct DeviceInfo
	{
		u16					StandardVersion;
		u32					VendorExtensionId;
		u16					VendorExtensionVersion;
		std::string			VendorExtensionDesc;
		u16					FunctionalMode;
		std::vector<u16>	OperationsSupported;
		std::vector<u16>	EventsSupported;
		std::vector<u16>	DevicePropertiesSupported;
		std::vector<u16>	CaptureFormats;
		std::vector<u16>	ImageFormats;
		std::string			Manufactorer;
		std::string			Model;
		std::string			DeviceVersion;
		std::string			SerialNumber;

		void Read(Stream &stream)
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
			stream >> Manufactorer;
			stream >> Model;
			stream >> DeviceVersion;
			stream >> SerialNumber;
		}
	};

	struct ObjectHandles
	{
		std::vector<u32> ObjectHandles;

		void Read(Stream &stream)
		{
			stream >> ObjectHandles;
		}
	};

	struct StorageIDs
	{
		std::vector<u32> StorageIDs;

		void Read(Stream &stream)
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


		void Read(Stream &stream)
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
		u32			StorageId;
		u16			ObjectFormat;
		u16			ProtectinStatus;
		u32			ObjectCompressedSize;
		u16			ThumbFormat;
		u32			ThumbCompressedSize;
		u32			ThumbPixWidth;
		u32			ThumbPixHeight;
		u32			ImagePixWidth;
		u32			ImagePixHeight;
		u32			ImageBitDepth;
		u32			ParentObject;
		u16			AssociationType;
		u32			AssociationDesc;
		u32			SequenceNumber;
		std::string	Filename;
		std::string	CaptureDate;
		std::string	ModificationDate;
		std::string	Keywords;
	};

}}


#endif	/* MESSAGES_H */

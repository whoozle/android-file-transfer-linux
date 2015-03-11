#ifndef MESSAGES_H
#define	MESSAGES_H

#include <mtp/ptp/Stream.h>

namespace mtp
{
	struct GetDeviceInfo
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

	struct GetObjectHandles
	{
		std::vector<u32> ObjectHandles;

		void Read(Stream &stream)
		{
			stream >> ObjectHandles;
		}
	};

	struct GetStorageIDs
	{
		std::vector<u32> StorageIDs;

		void Read(Stream &stream)
		{
			stream >> StorageIDs;
		}
	};

	struct GetStorageInfo
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

}


#endif	/* MESSAGES_H */

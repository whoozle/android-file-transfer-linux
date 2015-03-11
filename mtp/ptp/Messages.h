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
}


#endif	/* MESSAGES_H */

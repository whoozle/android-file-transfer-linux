#include <mtp/ptp/Messages.h>
#include <algorithm>
#include <sstream>
#include <ctype.h>

namespace mtp { namespace msg
{
	bool DeviceInfo::Supports(OperationCode opcode) const
	{
		auto i = std::find(OperationsSupported.begin(), OperationsSupported.end(), opcode);
		return i != OperationsSupported.end();
	}

	bool DeviceInfo::Supports(DeviceProperty property) const
	{
		auto i = std::find(DevicePropertiesSupported.begin(), DevicePropertiesSupported.end(), property);
		return i != DevicePropertiesSupported.end();
	}

	bool DeviceInfo::Supports(EventCode event) const
	{
		auto i = std::find(EventsSupported.begin(), EventsSupported.end(), event);
		return i != EventsSupported.end();
	}

	bool DeviceInfo::Supports(ObjectFormat format) const
	{
		auto i = std::find(ImageFormats.begin(), ImageFormats.end(), format);
		return i != ImageFormats.end();
	}

	namespace
	{
		std::string Strip(std::string str)
		{
			str.erase(std::remove_if(str.begin(), str.end(), isspace), str.end());
			return str;
		}
	}

	std::string DeviceInfo::GetFilesystemFriendlyName() const
	{
		std::stringstream ss;
		ss << Strip(Manufacturer);
		ss << '-';
		ss << Strip(Model);
		ss << '-';
		ss << Strip(SerialNumber);
		return ss.str();
	}

}}

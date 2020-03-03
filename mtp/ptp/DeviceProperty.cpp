#include <mtp/ptp/DeviceProperty.h>
#include <mtp/log.h>

namespace mtp
{
	std::string ToString(DeviceProperty property)
	{
		switch(property)
		{
#			define ENUM_VALUE(NAME, VALUE) ENUM_VALUE_TO_STRING(DeviceProperty, NAME, VALUE)
#			include <mtp/ptp/DeviceProperty.values.h>
			ENUM_VALUE_TO_STRING_DEFAULT(DeviceProperty, property, 4);
#			undef ENUM_VALUE
		}
	}

	std::string ToString(PerceivedDeviceType property)
	{
		switch(property)
		{
#			define ENUM_VALUE(NAME, VALUE) ENUM_VALUE_TO_STRING(PerceivedDeviceType, NAME, VALUE)
#			include <mtp/ptp/PerceivedDeviceType.values.h>
			ENUM_VALUE_TO_STRING_DEFAULT(PerceivedDeviceType, property, 8);
#			undef ENUM_VALUE
		}
	}
}

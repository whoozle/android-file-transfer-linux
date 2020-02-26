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
		}
	}
}

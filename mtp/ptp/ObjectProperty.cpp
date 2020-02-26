#include <mtp/ptp/ObjectProperty.h>
#include <mtp/log.h>

namespace mtp
{
	std::string ToString(ObjectProperty property)
	{
		switch(property)
		{
#			define ENUM_VALUE(NAME, VALUE) ENUM_VALUE_TO_STRING(ObjectProperty, NAME, VALUE)
#			include <mtp/ptp/ObjectProperty.values.h>
			ENUM_VALUE_TO_STRING_DEFAULT(ObjectProperty, property, 4);
		}
	}
}

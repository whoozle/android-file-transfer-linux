#include <mtp/ptp/DataTypeCode.h>
#include <mtp/log.h>

namespace mtp
{
	std::string ToString(DataTypeCode property)
	{
		switch(property)
		{
#			define ENUM_VALUE(NAME, VALUE) ENUM_VALUE_TO_STRING(DataTypeCode, NAME, VALUE)
#			include <mtp/ptp/DataTypeCode.values.h>
			ENUM_VALUE_TO_STRING_DEFAULT(DataTypeCode, property, 4);
		}
	}

}

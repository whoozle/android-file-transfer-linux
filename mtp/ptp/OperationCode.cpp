#include <mtp/ptp/OperationCode.h>
#include <mtp/log.h>

namespace mtp
{
	std::string ToString(OperationCode property)
	{
		switch(property)
		{
#			define ENUM_VALUE(NAME, VALUE) ENUM_VALUE_TO_STRING(OperationCode, NAME, VALUE)
#			include <mtp/ptp/OperationCode.values.h>
			ENUM_VALUE_TO_STRING_DEFAULT(OperationCode, property, 4);
		}
	}
}

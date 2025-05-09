#include <mtp/ptp/EventCode.h>
#include <mtp/log.h>

namespace mtp
{
	std::string ToString(EventCode code)
	{
		switch(code)
		{
#			define ENUM_VALUE(NAME, VALUE) ENUM_VALUE_TO_STRING(EventCode, NAME, VALUE)
#			include <mtp/ptp/EventCode.values.h>
			ENUM_VALUE_TO_STRING_DEFAULT(OperationCode, code, 4);
		}
	}
}

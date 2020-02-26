#include <mtp/ptp/DataTypeCode.h>
#include <mtp/log.h>

namespace mtp
{
	std::string ToString(DataTypeCode type)
	{
		switch(type)
		{
#			define ENUM_VALUE(NAME, VALUE) ENUM_VALUE_TO_STRING(DataTypeCode, NAME, VALUE)
#			include <mtp/ptp/DataTypeCode.values.h>
			ENUM_VALUE_TO_STRING_DEFAULT(DataTypeCode, type, 4);
		}
	}
	bool IsArray(DataTypeCode type)
	{
		switch(type)
		{
#			define CASE(BITS) case DataTypeCode::ArrayInt##BITS :  case DataTypeCode::ArrayUint##BITS :
			CASE(8)
			CASE(16)
			CASE(32)
			CASE(64)
			CASE(128)
				return true;
			default:
				return false;
		}
	}

}

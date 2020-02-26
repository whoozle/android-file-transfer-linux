#ifndef AFTL_MTP_PTP_DATATYPECODE_H
#define AFTL_MTP_PTP_DATATYPECODE_H

#include <mtp/types.h>
#include <string>

namespace mtp
{
	enum struct DataTypeCode : u16
	{
#define ENUM_VALUE(NAME, VALUE) ENUM_VALUE_DECL(NAME, VALUE)
#		include <mtp/ptp/DataTypeCode.values.h>
#undef ENUM_VALUE
	};
	DECLARE_ENUM(DataTypeCode, u16);

	bool IsArray(DataTypeCode type);
	std::string ToString(DataTypeCode type);

}

#endif

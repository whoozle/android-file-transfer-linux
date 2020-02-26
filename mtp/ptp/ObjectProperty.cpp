#include <mtp/ptp/ObjectProperty.h>
#include <mtp/ptp/InputStream.h>
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
	namespace
	{
		void ToString(std::stringstream & ss, DataTypeCode type, const ByteArray & value)
		{
			InputStream is(value);
			if (IsArray(type))
			{
				ss << "[";
				InputStream is(value);
				u32 size = is.Read32();

				while(size--)
				{
					switch(type)
					{
#define CASE(BITS) \
					case DataTypeCode::ArrayUint##BITS: \
					case DataTypeCode::ArrayInt##BITS: \
						ss << is.Read##BITS (); if (size) ss << ", "; break;
					CASE(8); CASE(16); CASE(32); CASE(64); CASE(128);
#undef CASE
					default:
						ss << "(value of unknown type " << ToString(type) << ")";
					}
				}
				ss << "]";
			}
			else
			{
				switch(type)
				{
#define CASE(BITS) \
					case DataTypeCode::Uint##BITS: \
					case DataTypeCode::Int##BITS: \
						ss << is.Read##BITS (); break;
					CASE(8); CASE(16); CASE(32); CASE(64); CASE(128);
#undef CASE
					case mtp::DataTypeCode::String:
						ss << '"' << is.ReadString() << '"'; break;
					default:
						ss << "(value of unknown type " << ToString(type) << ")";
				}
			}
		}
	}

	std::string ToString(DataTypeCode type, const ByteArray & value)
	{
		std::stringstream ss;
		ToString(ss, type, value);
		return ss.str();
	}

}

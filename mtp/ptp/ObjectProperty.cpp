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
		template <typename T>
		void ArrayToString(std::stringstream & ss, InputStream & is, u32 size)
		{
			ss << "[ ";
			while(size--) {
				T value;
				is >> value;
				ss << value;
				if (size)
					ss << " ";
			}
			ss << "]";
		}

		void ToString(std::stringstream & ss, InputStream & is, DataTypeCode type)
		{
			if (IsArray(type))
			{
				u32 size = is.Read32();

				switch(type)
				{
#define CASE(CODE, TYPE) case CODE : ArrayToString<TYPE>(ss, is, size); break
				CASE(DataTypeCode::ArrayInt8, s8);
				CASE(DataTypeCode::ArrayInt16, s16);
				CASE(DataTypeCode::ArrayInt32, s32);
				CASE(DataTypeCode::ArrayInt64, s64);
				CASE(DataTypeCode::ArrayUint16, u16);
				CASE(DataTypeCode::ArrayUint32, u32);
				CASE(DataTypeCode::ArrayUint64, u64);
				case DataTypeCode::ArrayUint8:
					HexDump(ss, "value", size, is);
					break;
#undef CASE

				default:
					ss << "(value of unknown type " << ToString(type) << ")";
				}
			}
			else
			{
				switch(type)
				{
#define CASE(BITS) \
					case DataTypeCode::Uint##BITS: \
					case DataTypeCode::Int##BITS: \
						ss << is.Read##BITS (); break;
					CASE(8); CASE(16); CASE(32); CASE(64);
#undef CASE
					case mtp::DataTypeCode::Uint128:
					case mtp::DataTypeCode::Int128:
						HexDump(ss, "value", 16, is);
						break;
					case mtp::DataTypeCode::String:
						ss << is.ReadString(); break;
					default:
						ss << "(value of unknown type " << ToString(type) << ")";
				}
			}
		}

		bool IsString(const ByteArray & value)
		{
			return true;
		}
	}

	std::string ToString(DataTypeCode type, const ByteArray & value)
	{
		std::stringstream ss;
		InputStream is(value);
		if (type == DataTypeCode::ArrayUint16 && IsString(value))
		{
			u32 size = is.Read32();
			ss << is.ReadString(size);
		} else
			ToString(ss, is, type);
		return ss.str();
	}

}

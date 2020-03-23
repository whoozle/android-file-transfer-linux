/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2020  Vladimir Menshakov

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef AFTL_MTP_PTP_OBJECTPROPERTYLISTPARSER_H
#define AFTL_MTP_PTP_OBJECTPROPERTYLISTPARSER_H

#include <mtp/ptp/ObjectId.h>
#include <mtp/ptp/ObjectProperty.h>
#include <mtp/ByteArray.h>
#include <mtp/ptp/InputStream.h>
#include <functional>

namespace mtp
{

	namespace impl
	{
		template<typename PropertyType>
		struct ObjectPropertyParser
		{
			static PropertyType Parse(InputStream &stream, DataTypeCode dataType)
			{
				switch(dataType)
				{
#define HANDLE_TYPE(TYPE, METHOD) case DataTypeCode::TYPE : { return static_cast<PropertyType> (stream . METHOD ()) ; }
				HANDLE_TYPE(Uint8,  Read8)
				HANDLE_TYPE(Uint16, Read16)
				HANDLE_TYPE(Uint32, Read32)
				HANDLE_TYPE(Uint64, Read64)
				HANDLE_TYPE(Int8,   Read8)
				HANDLE_TYPE(Int16,  Read16)
				HANDLE_TYPE(Int32,  Read32)
				HANDLE_TYPE(Int64,  Read64)
#undef HANDLE_TYPE
				default:
					throw std::runtime_error("got invalid type");
				}
			}
		};

		template<>
		struct ObjectPropertyParser<std::string>
		{
			static std::string Parse(InputStream &stream, DataTypeCode dataType)
			{
				if (dataType != DataTypeCode::String)
					throw std::runtime_error("got invalid type");

				std::string value;
				stream >> value;
				return value;
			}

		};
	}

	template<typename PropertyValueType, template <typename> class Parser = impl::ObjectPropertyParser>
	struct ObjectPropertyListParser
	{
		static u32 GetSize(const ByteArray & data)
		{
			InputStream stream(data);
			u32 n;
			stream >> n;
			return n;
		}

		static void Parse(const ByteArray & data, const std::function<void (ObjectId, ObjectProperty property, const PropertyValueType &)> &func)
		{
			InputStream stream(data);
			u32 n;
			stream >> n;
			while(n--)
			{
				ObjectId objectId;
				ObjectProperty property;
				DataTypeCode dataType;

				stream >> objectId;
				stream >> property;
				stream >> dataType;

				PropertyValueType value = Parser<PropertyValueType>::Parse(stream, dataType);
				func(objectId, property, value);
			}
		}
	};

	using ObjectStringPropertyListParser = ObjectPropertyListParser<std::string>;
}

#endif

/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015  Vladimir Menshakov

    Android File Transfer For Linux is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    Android File Transfer For Linux is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Android File Transfer For Linux.
    If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AFS_MTP_PTP_OBJECTPROPERTYLISTPARSER_H
#define	AFS_MTP_PTP_OBJECTPROPERTYLISTPARSER_H

#include <mtp/ptp/ObjectProperty.h>
#include <mtp/ByteArray.h>
#include <mtp/ptp/InputStream.h>
#include <functional>

namespace mtp
{
	namespace impl
	{
		template<typename PropertyType>
		struct ObjectPropertyParser;

		template<>
		struct ObjectPropertyParser<std::string>
		{
			static std::string Parse(InputStream &stream)
			{
				DataTypeCode dataType;
				stream >> dataType;
				if (dataType != DataTypeCode::String)
					throw std::runtime_error("got invalid type");

				std::string value;
				stream >> value;
				return value;
			}

		};
	}

	template<typename PropertyValueType>
	struct ObjectPropertyListParser
	{
		void Parse(const ByteArray & data, const std::function<void (u32, const PropertyValueType &)> &func)
		{
			InputStream stream(data);
			u32 n;
			stream >> n;
			while(n--)
			{
				u32 objectId;
				ObjectProperty property;

				stream >> objectId;
				stream >> property;
				PropertyValueType value = impl::ObjectPropertyParser<PropertyValueType>::Parse(stream);
				func(objectId, value);
			}
		}
	};
}

#endif

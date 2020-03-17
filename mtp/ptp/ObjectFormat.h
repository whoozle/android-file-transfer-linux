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

#ifndef AFTL_MTP_PTP_OBJECTFORMAT_H
#define AFTL_MTP_PTP_OBJECTFORMAT_H

#include <mtp/types.h>
#include <string>
#include <time.h>

namespace mtp
{
	static const u64	MaxObjectSize = 0xffffffffull;

	//please look here: https://msdn.microsoft.com/en-us/library/jj584872(v=winembedded.70).aspx

	enum struct ObjectFormat : u16
	{
#define ENUM_VALUE(NAME, VALUE) ENUM_VALUE_DECL(NAME, VALUE)
#		include <mtp/ptp/ObjectFormat.values.h>
#undef ENUM_VALUE
	};

	DECLARE_ENUM(ObjectFormat, u16);
	std::string ToString(ObjectFormat property);

	enum struct AssociationType : u16
	{
		GenericFolder			= 0x0001,
		Album					= 0x0002,
		TimeSequence			= 0x0003,
		HorizontalPanoramic		= 0x0004,
		VerticalPanoramic		= 0x0005,
		Panoramic2D				= 0x0006,
		AncillaryData			= 0x0007
	};

	DECLARE_ENUM(AssociationType, u16);

	ObjectFormat ObjectFormatFromFilename(const std::string &filename);

	bool IsAudioFormat(ObjectFormat format);
	bool IsImageFormat(ObjectFormat format);

	time_t ConvertDateTime(const std::string &timespec);
	std::string ConvertDateTime(time_t);
	std::string ConvertYear(int year);

}

namespace std
{
	template<> struct hash<mtp::ObjectFormat>
	{ public: size_t operator()(mtp::ObjectFormat format) const { return static_cast<size_t>(format); } };
}

#endif

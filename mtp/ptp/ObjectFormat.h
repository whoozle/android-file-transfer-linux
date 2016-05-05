/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2016  Vladimir Menshakov

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

#ifndef OBJECTFORMAT_H
#define	OBJECTFORMAT_H

#include <mtp/types.h>
#include <string>
#include <time.h>

namespace mtp
{
	static const u64	MaxObjectSize = 0xffffffffull;

	//please look here: https://msdn.microsoft.com/en-us/library/jj584872(v=winembedded.70).aspx

	enum struct ObjectFormat : u16
	{
		Any				= 0x0000,
		Undefined		= 0x3000,
		Association		= 0x3001,
		Script			= 0x3002,
		Executable		= 0x3003,
		Text			= 0x3004,
		Html			= 0x3005,
		Dpof			= 0x3006,
		Aiff			= 0x3007,
		Wav				= 0x3008,
		Mp3				= 0x3009,
		Avi				= 0x300a,
		Mpeg			= 0x300b,
		Asf				= 0x300c,
		UndefinedImage	= 0x3800,
		ExifJpeg		= 0x3801,
		TiffEp			= 0x3802,
		Flashpix		= 0x3803,
		Bmp				= 0x3804,
		Ciff			= 0x3805,
		Reserved		= 0x3806,
		Gif				= 0x3807,
		Jfif			= 0x3808,
		Pcd				= 0x3809,
		Pict			= 0x380a,
		Png				= 0x380b,
		Reserved2		= 0x380c,
		Tiff			= 0x380d,
		TiffIt			= 0x380e,
		Jp2				= 0x380f,
		Jpx				= 0x3810,

		//audio
		Wma				= 0xb901,
		Ogg				= 0xb902,
		Aac				= 0xb903,
		Audible			= 0xb904,
		Flac			= 0xb906,

		//video
		Wmv				= 0xb980,
		Mp4				= 0xb982,
		Mp2				= 0xb983,
		_3gp			= 0xb984,

		AudioAlbum		= 0xba03,

		//playlists
		Wpl				= 0xba10,
		M3u				= 0xba11,
		Mpl				= 0xba12,
		Asx				= 0xba13,
		Pls				= 0xba14,

		Xml				= 0xba82,
		Doc				= 0xba83,
		Mht				= 0xba84,
		Xls				= 0xba85,
		Ppt				= 0xba86,

		VCard2			= 0xbb82
	};

	DECLARE_ENUM(ObjectFormat, u16);

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
	time_t ConvertDateTime(const std::string &timespec);
	std::string ConvertDateTime(time_t);

}

#endif

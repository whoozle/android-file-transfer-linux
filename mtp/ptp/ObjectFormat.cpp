/*
 * Android File Transfer for Linux: MTP client for android devices
 * Copyright (C) 2015  Vladimir Menshakov

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <mtp/ptp/ObjectFormat.h>
#include <algorithm>
#include <ctype.h>

namespace mtp
{
	ObjectFormat ObjectFormatFromFilename(const std::string &filename)
	{
		size_t extPos = filename.rfind('.');
		if (extPos == filename.npos)
			return ObjectFormat::Undefined;

		std::string ext = filename.substr(extPos + 1);
		std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
		if (ext == "mp3")
			return mtp::ObjectFormat::Mp3;
		else if (ext == "txt")
			return mtp::ObjectFormat::Text;
		else if (ext == "jpeg" || ext == "jpg")
			return mtp::ObjectFormat::Jfif;
		else if (ext == "gif")
			return mtp::ObjectFormat::Gif;
		else if (ext == "bmp")
			return mtp::ObjectFormat::Bmp;
		else if (ext == "png")
			return mtp::ObjectFormat::Png;
		else if (ext == "wma")
			return mtp::ObjectFormat::Wma;
		else if (ext == "ogg")
			return mtp::ObjectFormat::Ogg;
		else if (ext == "flac")
			return mtp::ObjectFormat::Flac;
		else if (ext == "aac")
			return mtp::ObjectFormat::Aac;
		else if (ext == "wav")
			return mtp::ObjectFormat::Aiff;
		else if (ext == "wmv")
			return mtp::ObjectFormat::Wmv;
		else if (ext == "mp4")
			return mtp::ObjectFormat::Mp4;
		else if (ext == "3gp")
			return mtp::ObjectFormat::_3gp;
		else if (ext == "asf")
			return mtp::ObjectFormat::Asf;
		else if (ext == "m3u")
			return mtp::ObjectFormat::M3u;
		else
			return ObjectFormat::Undefined;
	}

	time_t ConvertDateTime(const std::string &timespec)
	{
		struct tm time = {};
		char *end = strptime(timespec.c_str(), "%Y%m%dT%H%M%S", &time);
		if (!end)
			return 0;
		return mktime(&time);
	}
}

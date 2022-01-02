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

#include <mtp/ptp/ObjectFormat.h>
#include <mtp/log.h>
#include <algorithm>
#include <ctype.h>
#include <map>

#ifdef HAVE_LIBMAGIC
#	include <magic.h>
#endif

namespace mtp
{
	namespace
	{
		std::string GetExtension(const std::string &filename)
		{
			size_t extPos = filename.rfind('.');
			std::string ext = (extPos != filename.npos)? filename.substr(extPos + 1): std::string();
			std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
			return ext;
		}
	}

#ifdef HAVE_LIBMAGIC
	namespace
	{
		class Magic
		{
			magic_t								_magic;
			std::map<std::string, ObjectFormat>	_types;

		public:
			Magic(): _magic(magic_open(MAGIC_MIME_TYPE | MAGIC_SYMLINK | MAGIC_ERROR))
			{
#define MAP_TYPE(name, format) _types[name] = (format)
				magic_load(_magic, NULL);
				MAP_TYPE("inode/directory",		ObjectFormat::Association);
				MAP_TYPE("audio/mpeg",			ObjectFormat::Mp3);
				MAP_TYPE("text/plain",			ObjectFormat::Text);
				MAP_TYPE("image/jpeg",			ObjectFormat::ExifJpeg);
				MAP_TYPE("image/gif",			ObjectFormat::Gif);
				MAP_TYPE("image/x-ms-bmp",		ObjectFormat::Bmp);
				MAP_TYPE("image/png",			ObjectFormat::Png);
				MAP_TYPE("audio/x-ms-wma",		ObjectFormat::Wma);
				MAP_TYPE("audio/ogg",			ObjectFormat::Ogg);
				MAP_TYPE("audio/x-flac",		ObjectFormat::Flac);
				MAP_TYPE("audio/x-m4a",			ObjectFormat::Aac);
				MAP_TYPE("audio/audio/x-wav",	ObjectFormat::Aiff);
				MAP_TYPE("audio/mp4",			ObjectFormat::Mp4);
				MAP_TYPE("application/x-mpegurl", ObjectFormat::M3uPlaylist);
#undef MAP_TYPE
			}

			ObjectFormat GetType(const std::string &path)
			{
				const char *type = _magic? magic_file(_magic, path.c_str()): NULL;
				if (!type)
					return ObjectFormat::Undefined;

				//debug("MAGIC MIME: ", type);
				auto it = _types.find(type);
				return it != _types.end()? it->second: ObjectFormat::Undefined;
			}

			~Magic()
			{ if (_magic) magic_close(_magic); }
		};
	}
#else
	namespace
	{
		struct Magic
		{
			const ObjectFormat GetType(const std::string &) { return ObjectFormat::Undefined; }
		};
	}
#endif

	ObjectFormat ObjectFormatFromFilename(const std::string &filename)
	{
		//libmagic missing mime type for m3u files
		auto ext = GetExtension(filename);
		if (ext == "m3u")
			return mtp::ObjectFormat::M3uPlaylist;

		static Magic magic;
		{
			ObjectFormat magicType = magic.GetType(filename);
			if (magicType != ObjectFormat::Undefined)
				return magicType;
		}

		if (ext == "mp3")
			return mtp::ObjectFormat::Mp3;
		else if (ext == "txt")
			return mtp::ObjectFormat::Text;
		else if (ext == "jpeg" || ext == "jpg")
			return mtp::ObjectFormat::ExifJpeg;
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
		else if (ext == "m4a")
			return mtp::ObjectFormat::M4a;
		else if (ext == "3gp")
			return mtp::ObjectFormat::_3gp;
		else if (ext == "asf")
			return mtp::ObjectFormat::Asf;
		else
			return ObjectFormat::Undefined;
	}

	bool IsAudioFormat(ObjectFormat format)
	{
		switch(format)
		{
			case ObjectFormat::Aiff:
			case ObjectFormat::Wav:
			case ObjectFormat::Mp3:
			case ObjectFormat::M4a:
			case ObjectFormat::UndefinedAudio:
			case ObjectFormat::Wma:
			case ObjectFormat::Ogg:
			case ObjectFormat::Aac:
			case ObjectFormat::Audible:
			case ObjectFormat::Flac:
				return true;
			default:
				return false;
		}
	}

	bool IsImageFormat(ObjectFormat format)
	{
		u16 type = static_cast<u16>(format) >> 8;
		return type == 0x38;
	}


	time_t ConvertDateTime(const std::string &timespec)
	{
		struct tm time = {};
		time.tm_isdst = -1;
		char *end = strptime(timespec.c_str(), "%Y%m%dT%H%M%S", &time);
		if (!end)
			return 0;
		return mktime(&time);
	}

	std::string ConvertDateTime(time_t time)
	{
		struct tm bdt = {};
		if (!gmtime_r(&time, &bdt))
			throw std::runtime_error("gmtime_r failed");
		char buf[64];
		size_t r = strftime(buf, sizeof(buf), "%Y%m%dT%H%M%SZ", &bdt);
		return std::string(buf, r);
	}

	std::string ConvertYear(int year)
	{
		struct tm bdt = {};
		bdt.tm_mday = 1;
		bdt.tm_year = year - 1900;
		auto ts = mktime(&bdt);
		if (ts == (time_t) -1)
			throw std::runtime_error("mktime failed");
		return ConvertDateTime(ts);
	}

	std::string ToString(ObjectFormat property)
	{
		switch(property)
		{
#			define ENUM_VALUE(NAME, VALUE) ENUM_VALUE_TO_STRING(ObjectFormat, NAME, VALUE)
#			include <mtp/ptp/ObjectFormat.values.h>
			ENUM_VALUE_TO_STRING_DEFAULT(ObjectFormat, property, 4);
		}
	}

}

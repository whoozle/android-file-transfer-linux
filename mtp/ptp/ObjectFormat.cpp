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
}

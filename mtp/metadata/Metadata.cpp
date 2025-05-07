#include <mtp/metadata/Metadata.h>
#include <mtp/log.h>

#ifdef HAVE_TAGLIB
#	include <fileref.h>
#	include <tag.h>
#	include <tpropertymap.h>
#endif

namespace mtp
{
#ifdef HAVE_TAGLIB
	MetadataPtr Metadata::Read(const std::string & path)
	{
		TagLib::FileRef f(path.c_str());
		auto tag = f.tag();
		if (f.isNull() || !tag)
			return nullptr;

		auto meta = std::make_shared<Metadata>();
		auto artist = tag->artist();

		const auto & props = tag->properties();
		auto album_it = props.find("ALBUMARTIST");
		if (album_it == props.end())
			album_it = props.find("ALBUM ARTIST");
		if (album_it == props.end())
			album_it = props.find("MUSICBRAINZ_ALBUMARTIST");
		if (album_it != props.end())
			artist = album_it->second.toString();

		meta->Title 	= tag->title().to8Bit(true);
		meta->Artist 	= artist.to8Bit(true);
		meta->Album 	= tag->album().to8Bit(true);
		meta->Genre 	= tag->genre().to8Bit(true);
		meta->Year 		= tag->year();
		meta->Track		= tag->track();

#if TAGLIB_MAJOR_VERSION >= 2
		for(auto & props : tag->complexProperties("PICTURE"))
		{
			auto &picture = meta->Picture;
			for(auto &kv : props)
			{
				auto &name = kv.first;
				auto &value = kv.second;
				if (name == "data") {
					auto data = value.toByteVector();
					picture.Data.assign(data.begin(), data.end());
				} else if (name == "pictureType") {
					picture.Type = value.toString().to8Bit(true);
				} else if (name == "mimeType") {
					picture.MimeType = value.toString().to8Bit(true);
				} else if (name == "description") {
					picture.Description = value.toString().to8Bit(true);
				} else {
					mtp::debug("unhandled PICTURE property ", name.toCString(true));
				}
			}
		}
#endif
		return meta;
	}

#else
	MetadataPtr Metadata::Read(const std::string & path)
	{ return nullptr; }
#endif
}

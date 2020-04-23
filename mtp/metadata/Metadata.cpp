#include <mtp/metadata/Metadata.h>

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
		return meta;
	}

#else
	MetadataPtr Metadata::Read(const std::string & path)
	{ return nullptr; }
#endif
}

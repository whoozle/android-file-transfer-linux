#include <mtp/metadata/Metadata.h>

#include <fileref.h>
#include <tag.h>

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
		meta->Title 	= tag->title().to8Bit(true);
		meta->Artist 	= tag->artist().to8Bit(true);
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

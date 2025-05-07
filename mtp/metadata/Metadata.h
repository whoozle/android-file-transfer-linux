#ifndef AFTL_MTP_METADATA_METADATA_H
#define AFTL_MTP_METADATA_METADATA_H

#include <mtp/types.h>
#include <mtp/ByteArray.h>
#include <memory>
#include <string>

namespace mtp
{
	struct Metadata;
	DECLARE_PTR(Metadata);

	struct MetadataPicture
	{
		std::string     Type;
		std::string		MimeType;
		std::string		Description;
		mtp::ByteArray	Data;
	};

	struct Metadata
	{
		std::string 	Title;
		std::string 	Artist;
		std::string 	Album;
		std::string 	Genre;
		unsigned		Year;
		unsigned		Track;
		MetadataPicture Picture;

		static MetadataPtr Read(const std::string & path);
	};

}

#endif

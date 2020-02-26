#ifndef AFTL_MTP_METADATA_METADATA_H
#define AFTL_MTP_METADATA_METADATA_H

#include <mtp/types.h>
#include <memory>
#include <string>

namespace mtp
{
	struct Metadata;
	DECLARE_PTR(Metadata);

	struct Metadata
	{
		std::string 	Title;
		std::string 	Artist;
		std::string 	Album;
		std::string 	Genre;
		unsigned		Year;
		unsigned		Track;

		static MetadataPtr Read(const std::string & path);
	};

}

#endif

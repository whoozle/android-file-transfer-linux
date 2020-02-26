#ifndef AFTL_MTP_METADATA_LIBRARY_H
#define AFTL_MTP_METADATA_LIBRARY_H

#include <mtp/types.h>
#include <mtp/ptp/Session.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace mtp
{
	class Library
	{
		using ObjectNameMap = std::unordered_map<std::string, mtp::ObjectId>;

		SessionPtr		_session;
		ObjectNameMap 	_artists, _albums;

	public:
		Library(const mtp::SessionPtr & session);

		ObjectId GetArtist(const std::string & name)
		{ auto it = _artists.find(name); return it != _artists.end()? it->second: ObjectId(); }

		ObjectId GetAlbum(const std::string & name)
		{ auto it = _albums.find(name); return it != _albums.end()? it->second: ObjectId(); }
	};
}

#endif

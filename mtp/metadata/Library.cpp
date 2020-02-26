#include <mtp/metadata/Library.h>
#include <mtp/log.h>

namespace mtp
{

	Library::Library(const mtp::SessionPtr & session): _session(session)
	{
		using namespace mtp;
		{
			auto artists = _session->GetObjectHandles(Session::AllStorages, ObjectFormat::Artist, Session::Device);
			for (auto id : artists.ObjectHandles)
			{
				auto name = _session->GetObjectStringProperty(id, ObjectProperty::Name);
				debug("artist: ", name, "\t", id.Id);
				auto artist = std::make_shared<Artist>();
				artist->Id = id;
				artist->Name = name;
				_artists.insert(std::make_pair(name, artist));
			}
		}
		{
			auto albums = _session->GetObjectHandles(Session::AllStorages, ObjectFormat::AudioAlbum, Session::Device);
			for (auto id : albums.ObjectHandles)
			{
				auto name = _session->GetObjectStringProperty(id, ObjectProperty::Name);
				auto artistName = _session->GetObjectStringProperty(id, ObjectProperty::Artist);
				auto artist = GetArtist(artistName);
				if (!artist)
					error("invalid artist name in album ", name);

				debug("album: ", name, "\t", id.Id);
				auto album = std::make_shared<Album>();
				album->Name = name;
				album->Id = id;
				album->Year = 0;
				_albums.insert(std::make_pair(std::make_pair(artist, name), album));
			}
		}
	}

	Library::ArtistPtr Library::CreateArtist(const std::string & name)
	{
		if (name.empty())
			return nullptr;

		ByteArray propList;
		OutputStream os(propList);

		os.Write32(1); //number of props
		os.Write32(0); //object handle
		os.Write16(static_cast<u16>(ObjectProperty::Name));
		os.Write16(static_cast<u16>(DataTypeCode::String));
		os.WriteString(name);

		auto response = _session->SendObjectPropList(Session::AnyStorage, Session::Device, ObjectFormat::Artist, 0, propList);
		auto artist = std::make_shared<Artist>();
		artist->Id = response.ObjectId;
		artist->Name = name;
		_artists.insert(std::make_pair(name, artist));
		return artist;
	}

	Library::AlbumPtr Library::CreateAlbum(const ArtistPtr & artist, const std::string & name)
	{
		if (name.empty() || !artist)
			return nullptr;

		ByteArray propList;
		OutputStream os(propList);

		os.Write32(2); //number of props

		os.Write32(0); //object handle
		os.Write16(static_cast<u16>(ObjectProperty::Name));
		os.Write16(static_cast<u16>(DataTypeCode::String));
		os.WriteString(name);

		os.Write32(0); //object handle
		os.Write16(static_cast<u16>(ObjectProperty::Artist));
		os.Write16(static_cast<u16>(DataTypeCode::String));
		os.WriteString(artist->Name);

		// os.Write32(0); //object handle
		// os.Write16(static_cast<u16>(ObjectProperty::ArtistId));
		// os.Write16(static_cast<u16>(DataTypeCode::Uint32));
		// os.Write32(artist->Id.Id);

		auto response = _session->SendObjectPropList(Session::AnyStorage, Session::Root, ObjectFormat::AudioAlbum, 0, propList);

		auto album = std::make_shared<Album>();
		album->Id = response.ObjectId;
		album->Artist = artist;
		album->Name = name;
		_albums.insert(std::make_pair(std::make_pair(artist, name), album));
		return album;
	}

}

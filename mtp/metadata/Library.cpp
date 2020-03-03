#include <mtp/metadata/Library.h>
#include <mtp/ptp/Session.h>
#include <mtp/log.h>
#include <unordered_map>

namespace mtp
{

	Library::Library(const mtp::SessionPtr & session): _session(session)
	{
		auto storages = _session->GetStorageIDs();
		if (storages.StorageIDs.empty())
			throw std::runtime_error("no storages found");

		_storage = storages.StorageIDs[0]; //picking up first storage.
		//zune fails to create artist/album without storage id

		{
			msg::ObjectHandles rootFolders = _session->GetObjectHandles(Session::AllStorages, mtp::ObjectFormat::Association, Session::Root);
			for (auto id : rootFolders.ObjectHandles)
			{
				auto name = _session->GetObjectStringProperty(id, ObjectProperty::ObjectFilename);
				if (name == "Artists")
					_artistsFolder = id;
				else if (name == "Albums")
					_albumsFolder = id;
				else if (name == "Music")
					_musicFolder = id;
			}
		}
		if (_artistsFolder == ObjectId())
			_artistsFolder = _session->CreateDirectory("Artists", Session::Root, _storage).ObjectId;
		if (_albumsFolder == ObjectId())
			_albumsFolder = _session->CreateDirectory("Albums", Session::Root, _storage).ObjectId;
		if (_musicFolder == ObjectId())
			_musicFolder = _session->CreateDirectory("Music", Session::Root, _storage).ObjectId;

		debug("artists folder: ", _artistsFolder.Id);
		debug("albums folder: ", _albumsFolder.Id);
		debug("music folder: ", _musicFolder.Id);

		std::unordered_map<std::string, ObjectId> musicFolders;
		{
			debug("reading music folders");
			msg::ObjectHandles folders = _session->GetObjectHandles(_storage, mtp::ObjectFormat::Association, _musicFolder);
			musicFolders.reserve(folders.ObjectHandles.size());

			for(auto id : folders.ObjectHandles)
			{
				auto name = _session->GetObjectStringProperty(id, ObjectProperty::ObjectFilename);
				musicFolders.insert(std::make_pair(name, id));
			}
		}

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
			auto albums = _session->GetObjectHandles(Session::AllStorages, ObjectFormat::AbstractAudioAlbum, Session::Device);
			for (auto id : albums.ObjectHandles)
			{
				auto name = _session->GetObjectStringProperty(id, ObjectProperty::Name);
				auto artistName = _session->GetObjectStringProperty(id, ObjectProperty::Artist);
				auto albumDate = _session->GetObjectStringProperty(id, ObjectProperty::DateAuthored);
				auto artist = GetArtist(artistName);
				if (!artist)
					error("invalid artist name in album ", name);

				debug("album: ", name, "\t", id.Id, "\t", albumDate);
				auto album = std::make_shared<Album>();
				album->Name = name;
				album->Id = id;
				album->Year = ConvertDateTime(albumDate);
				_albums.insert(std::make_pair(std::make_pair(artist, name), album));
			}
		}
	}

	Library::~Library()
	{ }

	Library::ArtistPtr Library::CreateArtist(const std::string & name)
	{
		if (name.empty())
			return nullptr;

		ByteArray propList;
		OutputStream os(propList);

		os.Write32(2); //number of props

		os.Write32(0); //object handle
		os.Write16(static_cast<u16>(ObjectProperty::Name));
		os.Write16(static_cast<u16>(DataTypeCode::String));
		os.WriteString(name);

		os.Write32(0); //object handle
		os.Write16(static_cast<u16>(ObjectProperty::ObjectFilename));
		os.Write16(static_cast<u16>(DataTypeCode::String));
		os.WriteString(name + ".art");

		auto response = _session->SendObjectPropList(_storage, _artistsFolder, ObjectFormat::Artist, 0, propList);
		auto artist = std::make_shared<Artist>();
		artist->Id = response.ObjectId;
		artist->Name = name;
		_artists.insert(std::make_pair(name, artist));
		return artist;
	}

	Library::AlbumPtr Library::CreateAlbum(const ArtistPtr & artist, const std::string & name, int year)
	{
		if (name.empty() || !artist)
			return nullptr;

		ByteArray propList;
		OutputStream os(propList);

		os.Write32(year? 4: 3); //number of props

		os.Write32(0); //object handle
		os.Write16(static_cast<u16>(ObjectProperty::ArtistId));
		os.Write16(static_cast<u16>(DataTypeCode::Uint32));
		os.Write32(artist->Id.Id);

		os.Write32(0); //object handle
		os.Write16(static_cast<u16>(ObjectProperty::Name));
		os.Write16(static_cast<u16>(DataTypeCode::String));
		os.WriteString(name);

		os.Write32(0); //object handle
		os.Write16(static_cast<u16>(ObjectProperty::ObjectFilename));
		os.Write16(static_cast<u16>(DataTypeCode::String));
		os.WriteString(artist->Name + "--" + name + ".alb");

		if (year)
		{
			os.Write32(0); //object handle
			os.Write16(static_cast<u16>(ObjectProperty::DateAuthored));
			os.Write16(static_cast<u16>(DataTypeCode::String));
			os.WriteString(ConvertYear(year));
		}

		auto response = _session->SendObjectPropList(_storage, _albumsFolder, ObjectFormat::AbstractAudioAlbum, 0, propList);

		auto album = std::make_shared<Album>();
		album->Id = response.ObjectId;
		album->Artist = artist;
		album->Name = name;
		_albums.insert(std::make_pair(std::make_pair(artist, name), album));
		return album;
	}

	ObjectId Library::CreateTrack(ArtistPtr artist, AlbumPtr album, ObjectFormat type, const std::string &name, const std::string &filename, size_t size)
	{
		ByteArray propList;
		OutputStream os(propList);

		os.Write32(3); //number of props

		os.Write32(0); //object handle
		os.Write16(static_cast<u16>(ObjectProperty::ArtistId));
		os.Write16(static_cast<u16>(DataTypeCode::Uint32));
		os.Write32(artist->Id.Id);

		os.Write32(0); //object handle
		os.Write16(static_cast<u16>(ObjectProperty::Name));
		os.Write16(static_cast<u16>(DataTypeCode::String));
		os.WriteString(name);

		os.Write32(0); //object handle
		os.Write16(static_cast<u16>(ObjectProperty::ObjectFilename));
		os.Write16(static_cast<u16>(DataTypeCode::String));
		os.WriteString(filename);

		auto response = _session->SendObjectPropList(Session::AnyStorage, Session::Device, type, size, propList);
		return response.ObjectId;
	}

}

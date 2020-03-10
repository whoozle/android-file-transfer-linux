#include <mtp/metadata/Library.h>
#include <mtp/ptp/Session.h>
#include <mtp/log.h>
#include <unordered_map>

namespace mtp
{
	namespace
	{
		const std::string UknownArtist	("UknownArtist");
		const std::string UknownAlbum	("UknownAlbum");
	}

	Library::NameToObjectIdMap Library::ListAssociations(ObjectId parentId)
	{
		NameToObjectIdMap list;
		auto folders = _session->GetObjectHandles(_storage, mtp::ObjectFormat::Association, parentId);
		list.reserve(folders.ObjectHandles.size());

		for(auto id : folders.ObjectHandles)
		{
			auto name = _session->GetObjectStringProperty(id, ObjectProperty::ObjectFilename);
			list.insert(std::make_pair(name, id));
		}
		return list;
	}

	ObjectId Library::GetOrCreate(ObjectId parentId, const std::string &name)
	{
		auto objects = _session->GetObjectHandles(_storage, mtp::ObjectFormat::Association, parentId);
		for (auto id : objects.ObjectHandles)
		{
			auto oname = _session->GetObjectStringProperty(id, ObjectProperty::ObjectFilename);
			if (name == oname)
				return id;
		}
		return _session->CreateDirectory(name, parentId, _storage).ObjectId;
	}

	Library::Library(const mtp::SessionPtr & session): _session(session)
	{
		auto storages = _session->GetStorageIDs();
		if (storages.StorageIDs.empty())
			throw std::runtime_error("no storages found");

		_artistSupported = _session->GetDeviceInfo().Supports(ObjectFormat::Artist);
		{
			auto propsSupported = _session->GetObjectPropertiesSupported(ObjectFormat::AbstractAudioAlbum);
			_albumDateAuthoredSupported = std::find(propsSupported.ObjectPropertyCodes.begin(), propsSupported.ObjectPropertyCodes.end(), ObjectProperty::DateAuthored) != propsSupported.ObjectPropertyCodes.end();
		}

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
		if (_artistSupported && _artistsFolder == ObjectId())
			_artistsFolder = _session->CreateDirectory("Artists", Session::Root, _storage).ObjectId;
		if (_albumsFolder == ObjectId())
			_albumsFolder = _session->CreateDirectory("Albums", Session::Root, _storage).ObjectId;
		if (_musicFolder == ObjectId())
			_musicFolder = _session->CreateDirectory("Music", Session::Root, _storage).ObjectId;

		debug("artists folder: ", _artistsFolder != ObjectId()? _artistsFolder.Id: 0);
		debug("albums folder: ", _albumsFolder.Id);
		debug("music folder: ", _musicFolder.Id);

		auto musicFolders = ListAssociations(_musicFolder);

		using namespace mtp;
		if (_artistSupported)
		{
			debug("getting artists...");
			auto artists = _session->GetObjectHandles(Session::AllStorages, ObjectFormat::Artist, Session::Device);
			for (auto id : artists.ObjectHandles)
			{
				auto name = _session->GetObjectStringProperty(id, ObjectProperty::Name);
				debug("artist: ", name, "\t", id.Id);
				auto artist = std::make_shared<Artist>();
				artist->Id = id;
				artist->Name = name;
				auto it = musicFolders.find(name);
				if (it != musicFolders.end())
					artist->MusicFolderId = it->second;
				else
					artist->MusicFolderId = _session->CreateDirectory(name, _musicFolder, _storage).ObjectId;

				_artists.insert(std::make_pair(name, artist));
			}
		}

		std::unordered_map<ArtistPtr, NameToObjectIdMap> albumFolders;
		{
			debug("getting albums...");
			auto albums = _session->GetObjectHandles(Session::AllStorages, ObjectFormat::AbstractAudioAlbum, Session::Device);
			for (auto id : albums.ObjectHandles)
			{
				auto name = _session->GetObjectStringProperty(id, ObjectProperty::Name);
				auto artistName = _session->GetObjectStringProperty(id, ObjectProperty::Artist);
				std::string albumDate;

				if (_albumDateAuthoredSupported)
					albumDate = _session->GetObjectStringProperty(id, ObjectProperty::DateAuthored);

				auto artist = GetArtist(artistName);
				if (!artist)
					artist = CreateArtist(artistName);

				debug("album: ", name, "\t", id.Id, "\t", albumDate);
				auto album = std::make_shared<Album>();
				album->Name = name;
				album->Artist = artist;
				album->Id = id;
				album->Year = !albumDate.empty()? ConvertDateTime(albumDate): 0;
				if (albumFolders.find(artist) == albumFolders.end()) {
					albumFolders[artist] = ListAssociations(artist->MusicFolderId);
				}
				auto it = albumFolders.find(artist);
				if (it == albumFolders.end())
					throw std::runtime_error("no iterator after insert, internal error");

				{
					auto refs = _session->GetObjectReferences(id);
					for (auto id : refs.ObjectHandles)
						album->Refs.insert(id);
				}

				const auto & albums = it->second;
				auto alit = albums.find(name);
				if (alit != albums.end())
					album->MusicFolderId = alit->second;
				else
					album->MusicFolderId = _session->CreateDirectory(name, artist->MusicFolderId, _storage).ObjectId;

				_albums.insert(std::make_pair(std::make_pair(artist, name), album));
			}
		}
	}

	Library::~Library()
	{ }

	Library::ArtistPtr Library::GetArtist(std::string name)
	{
		if (name.empty())
			name = UknownArtist;

		auto it = _artists.find(name);
		return it != _artists.end()? it->second: ArtistPtr();
	}


	Library::ArtistPtr Library::CreateArtist(std::string name)
	{
		if (name.empty())
			name = UknownArtist;

		auto artist = std::make_shared<Artist>();
		artist->Name = name;
		artist->MusicFolderId = GetOrCreate(_musicFolder, name);

		if (_artistSupported)
		{
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
			artist->Id = response.ObjectId;
		}

		_artists.insert(std::make_pair(name, artist));
		return artist;
	}

	Library::AlbumPtr Library::GetAlbum(const ArtistPtr & artist, std::string name)
	{
		if (name.empty())
			name = UknownAlbum;

		auto it = _albums.find(std::make_pair(artist, name));
		return it != _albums.end()? it->second: AlbumPtr();
	}

	Library::AlbumPtr Library::CreateAlbum(const ArtistPtr & artist, std::string name, int year)
	{
		if (!artist)
			throw std::runtime_error("artists is required");

		if (name.empty())
			name = UknownAlbum;

		ByteArray propList;
		OutputStream os(propList);
		bool sendYear = year != 0 && _albumDateAuthoredSupported;

		os.Write32(3 + (sendYear? 1: 0)); //number of props

		if (_artistSupported)
		{
			os.Write32(0); //object handle
			os.Write16(static_cast<u16>(ObjectProperty::ArtistId));
			os.Write16(static_cast<u16>(DataTypeCode::Uint32));
			os.Write32(artist->Id.Id);
		}
		else
		{
			os.Write32(0); //object handle
			os.Write16(static_cast<u16>(ObjectProperty::Artist));
			os.Write16(static_cast<u16>(DataTypeCode::String));
			os.WriteString(artist->Name);
		}

		os.Write32(0); //object handle
		os.Write16(static_cast<u16>(ObjectProperty::Name));
		os.Write16(static_cast<u16>(DataTypeCode::String));
		os.WriteString(name);

		os.Write32(0); //object handle
		os.Write16(static_cast<u16>(ObjectProperty::ObjectFilename));
		os.Write16(static_cast<u16>(DataTypeCode::String));
		os.WriteString(artist->Name + "--" + name + ".alb");

		if (sendYear)
		{
			os.Write32(0); //object handle
			os.Write16(static_cast<u16>(ObjectProperty::DateAuthored));
			os.Write16(static_cast<u16>(DataTypeCode::String));
			os.WriteString(ConvertYear(year));
		}

		auto album = std::make_shared<Album>();
		album->Artist = artist;
		album->Name = name;
		album->Year = year;
		album->MusicFolderId = GetOrCreate(artist->MusicFolderId, name);

		auto response = _session->SendObjectPropList(_storage, _albumsFolder, ObjectFormat::AbstractAudioAlbum, 0, propList);
		album->Id = response.ObjectId;

		_albums.insert(std::make_pair(std::make_pair(artist, name), album));
		return album;
	}

	ObjectId Library::CreateTrack(const ArtistPtr & artist,
		const AlbumPtr & album,
		ObjectFormat type,
		std::string name, const std::string & genre, int trackIndex,
		const std::string &filename, size_t size)
	{
		ByteArray propList;
		OutputStream os(propList);

		os.Write32(3 + (!genre.empty()? 1: 0) + (trackIndex? 1: 0)); //number of props

		if (_artistSupported)
		{
			os.Write32(0); //object handle
			os.Write16(static_cast<u16>(ObjectProperty::ArtistId));
			os.Write16(static_cast<u16>(DataTypeCode::Uint32));
			os.Write32(artist->Id.Id);
		}
		else
		{
			os.Write32(0); //object handle
			os.Write16(static_cast<u16>(ObjectProperty::Artist));
			os.Write16(static_cast<u16>(DataTypeCode::String));
			os.WriteString(artist->Name);
		}


		os.Write32(0); //object handle
		os.Write16(static_cast<u16>(ObjectProperty::Name));
		os.Write16(static_cast<u16>(DataTypeCode::String));
		os.WriteString(name);

		if (trackIndex)
		{
			os.Write32(0); //object handle
			os.Write16(static_cast<u16>(ObjectProperty::Track));
			os.Write16(static_cast<u16>(DataTypeCode::Uint16));
			os.Write16(trackIndex);
		}

		if (!genre.empty())
		{
			os.Write32(0); //object handle
			os.Write16(static_cast<u16>(ObjectProperty::Genre));
			os.Write16(static_cast<u16>(DataTypeCode::String));
			os.WriteString(genre);
		}

		os.Write32(0); //object handle
		os.Write16(static_cast<u16>(ObjectProperty::ObjectFilename));
		os.Write16(static_cast<u16>(DataTypeCode::String));
		os.WriteString(filename);

		auto response = _session->SendObjectPropList(_storage, album->MusicFolderId, type, size, propList);
		return response.ObjectId;
	}

	bool Library::Supported(const mtp::SessionPtr & session)
	{
		auto & gdi = session->GetDeviceInfo();
		return
			gdi.Supports(OperationCode::SendObjectPropList) &&
			gdi.Supports(OperationCode::SetObjectReferences) &&
			gdi.Supports(ObjectFormat::AbstractAudioAlbum);
		;
	}

}

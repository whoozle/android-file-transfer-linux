#include <mtp/metadata/Library.h>
#include <mtp/ptp/Session.h>
#include <mtp/ptp/ObjectPropertyListParser.h>
#include <mtp/log.h>
#include <algorithm>
#include <unordered_map>

namespace mtp
{
	namespace
	{
		const std::string UknownArtist		("UknownArtist");
		const std::string UknownAlbum		("UknownAlbum");
		const std::string VariousArtists	("VariousArtists");
	}

	Library::NameToObjectIdMap Library::ListAssociations(ObjectId parentId)
	{
		NameToObjectIdMap list;

		ByteArray data = _session->GetObjectPropertyList(parentId, ObjectFormat::Association, ObjectProperty::ObjectFilename, 0, 1);
		ObjectPropertyListParser<std::string> parser;
		parser.Parse(data, [&](ObjectId id, ObjectProperty property, const std::string &name) {
			list.insert(std::make_pair(name, id));
		});
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

	Library::Library(const mtp::SessionPtr & session, ProgressReporter && reporter): _session(session)
	{
		auto storages = _session->GetStorageIDs();
		if (storages.StorageIDs.empty())
			throw std::runtime_error("no storages found");

		u64 progress = 0, total = 0;
		if (reporter)
			reporter(State::Initialising, progress, total);

		_artistSupported = _session->GetDeviceInfo().Supports(ObjectFormat::Artist);
		debug("device supports ObjectFormat::Artist: ", _artistSupported? "yes": "no");
		{
			auto propsSupported = _session->GetObjectPropertiesSupported(ObjectFormat::AbstractAudioAlbum);
			_albumDateAuthoredSupported = propsSupported.Supports(ObjectProperty::DateAuthored);
			_albumCoverSupported = propsSupported.Supports(ObjectProperty::RepresentativeSampleData);
			mtp::debug("abstract album supports date authored: ", _albumDateAuthoredSupported, ", cover: ", _albumCoverSupported);
		}

		_storage = storages.StorageIDs[0]; //picking up first storage.
		//zune fails to create artist/album without storage id
		{
			ByteArray data = _session->GetObjectPropertyList(Session::Root, ObjectFormat::Association, ObjectProperty::ObjectFilename, 0, 1);
			ObjectStringPropertyListParser::Parse(data, [&](ObjectId id, ObjectProperty property, const std::string &name)
			{
				if (name == "Artists")
					_artistsFolder = id;
				else if (name == "Albums")
					_albumsFolder = id;
				else if (name == "Music")
					_musicFolder = id;
			});
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

		ByteArray artists, albums;
		if (_artistSupported)
		{
			debug("getting artists...");
			if (reporter)
				reporter(State::QueryingArtists, progress, total);

			artists = _session->GetObjectPropertyList(Session::Root, mtp::ObjectFormat::Artist, mtp::ObjectProperty::Name, 0, 1);
			HexDump("artists", artists);

			total += ObjectStringPropertyListParser::GetSize(artists);
		}
		{
			debug("getting albums...");
			if (reporter)
				reporter(State::QueryingAlbums, progress, total);

			albums = _session->GetObjectPropertyList(Session::Root, mtp::ObjectFormat::AbstractAudioAlbum,  mtp::ObjectProperty::Name, 0, 1);
			HexDump("albums", artists);

			total += ObjectStringPropertyListParser::GetSize(albums);
		}

		if (_artistSupported)
		{
			if (reporter)
				reporter(State::LoadingArtists, progress, total);

			ObjectStringPropertyListParser::Parse(artists, [&](ObjectId id, ObjectProperty property, const std::string &name)
			{
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
				if (reporter)
					reporter(State::LoadingArtists, ++progress, total);
			});
		}

		if (reporter)
			reporter(State::LoadingAlbums, progress, total);

		std::unordered_map<ArtistPtr, NameToObjectIdMap> albumFolders;
		ObjectStringPropertyListParser::Parse(albums, [&](ObjectId id, ObjectProperty property, const std::string &name)
		{
			auto artistName = _session->GetObjectStringProperty(id, ObjectProperty::Artist);

			std::string albumDate;
			if (_albumDateAuthoredSupported)
				albumDate = _session->GetObjectStringProperty(id, ObjectProperty::DateAuthored);

			auto artist = GetArtist(artistName);
			if (!artist)
				artist = CreateArtist(artistName);

			debug("album: ", artistName, " -- ", name, "\t", id.Id, "\t", albumDate);
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

			const auto & albums = it->second;
			auto alit = albums.find(name);
			if (alit != albums.end())
				album->MusicFolderId = alit->second;
			else
				album->MusicFolderId = _session->CreateDirectory(name, artist->MusicFolderId, _storage).ObjectId;

			_albums.insert(std::make_pair(std::make_pair(artist, name), album));
			if (reporter)
				reporter(State::LoadingAlbums, ++progress, total);
		});

		if (reporter)
			reporter(State::Loaded, progress, total);
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

	bool Library::HasTrack(const AlbumPtr & album, const std::string &name, int trackIndex)
	{
		if (!album)
			return false;

		LoadRefs(album);

		auto & tracks = album->Tracks;
		auto range = tracks.equal_range(name);
		for(auto i = range.first; i != range.second; ++i)
		{
			if (i->second == trackIndex)
				return true;
		}

		return false;
	}

	Library::NewTrackInfo Library::CreateTrack(const ArtistPtr & artist,
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
		NewTrackInfo ti;
		ti.Id = response.ObjectId;
		ti.Name = name;
		ti.Index = trackIndex;
		return ti;
	}

	void Library::LoadRefs(AlbumPtr album)
	{
		if (!album || album->RefsLoaded)
			return;

		auto refs = _session->GetObjectReferences(album->Id).ObjectHandles;
		std::copy(refs.begin(), refs.end(), std::inserter(album->Refs, album->Refs.begin()));
		for(auto trackId : refs)
		{
			auto name = _session->GetObjectStringProperty(trackId, ObjectProperty::Name);
			auto index = _session->GetObjectIntegerProperty(trackId, ObjectProperty::Track);
			debug("[", index, "]: ", name);
			album->Tracks.insert(std::make_pair(name, index));
		}
		album->RefsLoaded = true;
	}

	void Library::AddTrack(AlbumPtr album, const NewTrackInfo & ti)
	{
		if (!album)
			return;

		LoadRefs(album);

		auto & refs = album->Refs;
		auto & tracks = album->Tracks;

		msg::ObjectHandles handles;
		std::copy(refs.begin(), refs.end(), std::back_inserter(handles.ObjectHandles));
		handles.ObjectHandles.push_back(ti.Id);
		_session->SetObjectReferences(album->Id, handles);
		refs.insert(ti.Id);
		tracks.insert(std::make_pair(ti.Name, ti.Index));
	}

	void Library::AddCover(AlbumPtr album, const mtp::ByteArray &data)
	{
		if (!album || !_albumCoverSupported)
			return;

		mtp::debug("sending ", data.size(), " bytes of album cover...");
		_session->SetObjectPropertyAsArray(album->Id, mtp::ObjectProperty::RepresentativeSampleData, data);
	}

	bool Library::Supported(const mtp::SessionPtr & session)
	{
		auto & gdi = session->GetDeviceInfo();
		return
			gdi.Supports(OperationCode::GetObjectPropList) &&
			gdi.Supports(OperationCode::SendObjectPropList) &&
			gdi.Supports(OperationCode::SetObjectReferences) &&
			gdi.Supports(ObjectFormat::AbstractAudioAlbum);
		;
	}

}

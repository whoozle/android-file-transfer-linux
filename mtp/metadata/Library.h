#ifndef AFTL_MTP_METADATA_LIBRARY_H
#define AFTL_MTP_METADATA_LIBRARY_H

#include <mtp/ptp/ObjectId.h>
#include <mtp/ptp/ObjectFormat.h>
#include <mtp/types.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace mtp
{
	class Session;
	DECLARE_PTR(Session);

	class Library
	{
		SessionPtr		_session;
		StorageId		_storage;

	public:
		struct Artist
		{
			ObjectId 		Id;
			ObjectId		MusicFolderId;
			std::string 	Name;
		};
		DECLARE_PTR(Artist);

		struct Album
		{
			ObjectId 		Id;
			ObjectId		MusicFolderId;
			ArtistPtr		Artist;
			std::string 	Name;
			time_t	 		Year;
		};
		DECLARE_PTR(Album);

	private:
		ObjectId _artistsFolder;
		ObjectId _albumsFolder;
		ObjectId _musicFolder;

		using ArtistMap = std::unordered_map<std::string, ArtistPtr>;
		ArtistMap _artists;

		using AlbumKey = std::pair<ArtistPtr, std::string>;
		struct AlbumKeyHash
		{ size_t operator() (const AlbumKey & key) const {
			return std::hash<ArtistPtr>()(key.first) + std::hash<std::string>()(key.second);
		}};

		using AlbumMap = std::unordered_map<AlbumKey, AlbumPtr, AlbumKeyHash>;
		AlbumMap _albums;

		using NameToObjectIdMap = std::unordered_map<std::string, ObjectId>;
		NameToObjectIdMap ListAssociations(ObjectId parentId);

	public:

		Library(const mtp::SessionPtr & session);
		~Library();

		//search by Metadata?
		ArtistPtr GetArtist(const std::string & name)
		{ auto it = _artists.find(name); return it != _artists.end()? it->second: ArtistPtr(); }
		ArtistPtr CreateArtist(const std::string & name);

		AlbumPtr GetAlbum(const ArtistPtr & artist, const std::string & name)
		{ auto it = _albums.find(std::make_pair(artist, name)); return it != _albums.end()? it->second: AlbumPtr(); }
		AlbumPtr CreateAlbum(const ArtistPtr & artist, const std::string & name, int year);
		ObjectId CreateTrack(ArtistPtr artist, AlbumPtr album, ObjectFormat type, const std::string &name, const std::string & genre, int trackIndex, const std::string &filename, size_t size);
	};
}

#endif

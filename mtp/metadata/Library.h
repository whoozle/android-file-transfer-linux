#ifndef AFTL_MTP_METADATA_LIBRARY_H
#define AFTL_MTP_METADATA_LIBRARY_H

#include <mtp/ptp/ObjectId.h>
#include <mtp/ptp/ObjectFormat.h>
#include <mtp/types.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

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
			std::unordered_set<ObjectId> Refs;
		};
		DECLARE_PTR(Album);

	private:
		ObjectId _artistsFolder;
		ObjectId _albumsFolder;
		ObjectId _musicFolder;
		bool _artistSupported;
		bool _albumDateAuthoredSupported;

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

		ObjectId GetOrCreate(ObjectId parentId, const std::string &name);

	public:

		Library(const mtp::SessionPtr & session);
		~Library();

		static bool Supported(const mtp::SessionPtr & session);

		//search by Metadata?
		ArtistPtr GetArtist(std::string name);
		ArtistPtr CreateArtist(std::string name);

		AlbumPtr GetAlbum(const ArtistPtr & artist, std::string name);
		AlbumPtr CreateAlbum(const ArtistPtr & artist, std::string name, int year);
		ObjectId CreateTrack(const ArtistPtr & artist, const AlbumPtr & album, ObjectFormat type, std::string name, const std::string & genre, int trackIndex, const std::string &filename, size_t size);
		void AddTrack(AlbumPtr album, ObjectId id);
	};
	DECLARE_PTR(Library);
}

#endif

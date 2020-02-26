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
				_artists.insert(std::make_pair(name, id));
			}
		}
		{
			auto albums = _session->GetObjectHandles(Session::AllStorages, ObjectFormat::AudioAlbum, Session::Device);
			for (auto id : albums.ObjectHandles)
			{
				auto name = _session->GetObjectStringProperty(id, ObjectProperty::Name);
				debug("album: ", name, "\t", id.Id);
				_albums.insert(std::make_pair(name, id));
			}
		}
	}
/*
		ByteArray propList;
		OutputStream os(propList);

		os.Write32(1); //number of props
		os.Write32(0); //object handle
		os.Write16(static_cast<u16>(ObjectProperty::ObjectFilename));
		os.Write16(static_cast<u16>(DataTypeCode::String));
		os.WriteString(path);
			ByteArray propList;
			{
				OutputStream os(propList);
				os.Write32(1); //number of props
				os.Write32(0); //object handle
				os.Write16(static_cast<u16>(ObjectProperty::ObjectFilename));
				os.Write16(static_cast<u16>(DataTypeCode::String));
				os.WriteString(name);
			}
			auto response = SendObjectPropList(storageId, parentId, ObjectFormat::Association, 0, propList);
*/
}

#ifndef AFT_MTP_PTP_MEMORYOBJECTSTREAM_H
#define AFT_MTP_PTP_MEMORYOBJECTSTREAM_H

#include <mtp/ByteArray.h>
#include <mtp/ptp/IObjectStream.h>

namespace mtp
{
	class MemoryObjectOutputStream :
		public IObjectOutputStream,
		public CancellableStream
	{
		ByteArrayPtr _data;

	public:
		MemoryObjectOutputStream(): _data(new ByteArray()) { }

		size_t Write(const u8 *data, size_t size) override
		{
			auto & storage = *_data;
			auto offset = storage.size();
			storage.resize(offset + size);
			std::copy(data, data + size, storage.data() + offset);
			return size;
		}

		const ByteArrayPtr & GetData() const
		{ return _data; }
	};
}

#endif

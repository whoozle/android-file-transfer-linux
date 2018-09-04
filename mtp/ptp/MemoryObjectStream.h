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
		ByteArray _data;

		size_t Write(const u8 *data, size_t size) override
		{
			auto offset = _data.size();
			_data.resize(offset + size);
			std::copy(data, data + size, _data.data() + offset);
			return size;
		}

		const ByteArray & GetData() const
		{ return _data; }
	};
}

#endif

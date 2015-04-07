#ifndef BYTEARRAYOBJECTSTREAM_H
#define	BYTEARRAYOBJECTSTREAM_H

#include <mtp/ptp/IObjectStream.h>
#include <mtp/ByteArray.h>

namespace mtp
{
	class ByteArrayObjectInputStream : public IObjectInputStream
	{
		ByteArray	_data;
		size_t		_offset;

	public:
		ByteArrayObjectInputStream(const ByteArray & data): _data(data), _offset(0) { }
		ByteArrayObjectInputStream(ByteArray && data): _data(data), _offset(0) { }

		const ByteArray &GetData() const
		{ return _data; }

		virtual size_t GetSize() const
		{ return _data.size(); }

		virtual size_t Read(u8 *data, size_t size)
		{
			size_t n = std::min(size, _data.size() - _offset);
			std::copy(_data.data() + _offset, _data.data() + _offset + n, data);
			_offset += n;
			return n;
		}
	};
	DECLARE_PTR(ByteArrayObjectInputStream);

	class ByteArrayObjectOutputStream : public IObjectOutputStream
	{
		ByteArray	_data;

	public:
		ByteArrayObjectOutputStream(): _data() { }

		const ByteArray &GetData() const
		{ return _data; }

		virtual size_t Write(const u8 *data, size_t size)
		{
			std::copy(data, data + size, std::back_inserter(_data));
			return size;
		}
	};
	DECLARE_PTR(ByteArrayObjectOutputStream);

	class FixedSizeByteArrayObjectOutputStream : public IObjectOutputStream
	{
		ByteArray	_data;
		size_t		_offset;

	public:
		FixedSizeByteArrayObjectOutputStream(size_t size): _data(size), _offset(0) { }

		const ByteArray &GetData() const
		{ return _data; }

		virtual size_t Write(const u8 *data, size_t size)
		{
			size_t n = std::min(size, _data.size() - _offset);
			std::copy(data, data + n, _data.data() + _offset);
			_offset += n;
			return n;
		}
	};
	DECLARE_PTR(FixedSizeByteArrayObjectOutputStream);

}

#endif	/* BYTEARRAYOBJECTSTREAM_H */

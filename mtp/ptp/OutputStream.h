#ifndef OUTPUTSTREAM_H
#define	OUTPUTSTREAM_H

#include <mtp/types.h>
#include <mtp/ByteArray.h>

namespace mtp
{

	class OutputStream
	{
		ByteArray	& _data;

	public:
		OutputStream(ByteArray &data): _data(data)
		{ _data.reserve(512); }

		ByteArray & GetData()
		{ return _data; }

		const ByteArray & GetData() const
		{ return _data; }

		void Write8(u8 value)
		{ _data.push_back(value); }

		void Write16(u16 value)
		{ Write8(value); Write8(value >> 8); }

		void Write32(u32 value)
		{ Write16(value); Write16(value >> 16); }

		void Write64(u32 value)
		{ Write32(value); Write32(value >> 16); }

		static size_t Utf8Length(const std::string &value)
		{
			size_t size = 0;
			for(char c: value)
			{
				if ((c & 0x80) == 0 || (c & 0xc0) != 0x80)
					++size;
			}
			return size;
		}

		void WriteString(const std::string &value)
		{
			size_t len = Utf8Length(value);
			if (len > 255)
				throw std::runtime_error("string is too big (only 255 chars allowed)");
			Write8(len);
			for(size_t i = 0, p = 0; i < len && p < value.size(); ++i)
			{
				u8 c0 = value[p++];
				u16 uni;
				if (c0 == 0xc0 || c0 == 0xc1 || c0 >= 0xf5)
				{
					uni = '?';
				}
				else if (c0 < 0x80)
				{
					uni = c0;
				}
				else
				{
					u8 c1 = value[p++];
					if (c0 >= 0xc2 && c0 <= 0xdf)
						uni = ((c0 & 0x1f) << 6) | (c1 & 0x3f);
					else
					{
						u8 c2 = value[p++];
						if (c0 >= 0xe0 && c0 <= 0xef)
							uni = ((c0 & 0x0f) << 12) | ((c1 & 0x3f) << 6) | (c2 & 0x3f);
						else
							uni = '?';
					}
				}
				Write16(uni);
			}
		}

		template<typename ElementType>
		void WriteArray(const std::vector<ElementType> &array)
		{
			Write32(array.size());
			for(const auto &el : array)
				(*this) << el;
		}
	};

	inline OutputStream & operator << (OutputStream &stream, u8 value)
	{ stream.Write8(value); return stream; }

	inline OutputStream & operator << (OutputStream &stream, u16 value)
	{ stream.Write16(value); return stream; }

	inline OutputStream & operator << (OutputStream &stream, u32 value)
	{ stream.Write32(value); return stream; }

	inline OutputStream & operator << (OutputStream &stream, u64 value)
	{ stream.Write64(value); return stream; }

	inline OutputStream & operator << (OutputStream &stream, const std::string &value)
	{ stream.WriteString(value); return stream; }

	template<typename ElementType>
	inline OutputStream & operator << (OutputStream &stream, const std::vector<ElementType> &value)
	{ stream.template WriteArray<ElementType>(); return stream; }

}

#endif

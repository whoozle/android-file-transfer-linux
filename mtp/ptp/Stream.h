#ifndef STREAM_H
#define	STREAM_H

#include <mtp/types.h>
#include <mtp/ByteArray.h>

namespace mtp
{
	namespace impl
	{
		template<typename S, typename T>
		struct Reader;

		template<typename Stream>
		struct Reader<Stream, u8>
		{ static void Read(Stream &s, u8 &ref) { ref = s.ReadByte(); } };

		template<typename Stream>
		struct Reader<Stream, u16>
		{ static void Read(Stream &s, u16 &ref)
			{
				u16 l, h;
				l = s.ReadByte();
				h = s.ReadByte() << 8;
				ref = l | h;
			}
		};

		template<typename Stream>
		struct Reader<Stream, u32>
		{ static void Read(Stream &s, u32 &ref)
			{
				u16 l, h;
				Reader<Stream, u16>::Read(s, l);
				Reader<Stream, u16>::Read(s, h);
				ref = ((u32)h << 16) | l;
			}
		};

		template<typename Stream>
		struct Reader<Stream, u64>
		{ static void Read(Stream &s, u64 &ref)
			{
				u32 l, h;
				Reader<Stream, u32>::Read(s, l);
				Reader<Stream, u32>::Read(s, h);
				ref = ((u64)h << 32) | l;
			}
		};

		template<typename Stream>
		struct Reader<Stream, std::string>
		{ static void Read(Stream &s, std::string &str)
			{
				u8 len = s.ReadByte();
				str.clear();
				while(len--)
				{
					u16 ch;
					Reader<Stream, u16>::Read(s, ch);
					if (ch <= 0x7f)
						str += (char)ch;
					else if (ch <= 0x7ff)
					{
						str += (char) ((ch >> 6) | 0xc0);
						str += (char) ((ch & 0x3f) | 0x80);
				    }
					else if (ch <= 0xffff)
					{
						str += (char)((ch >> 12) | 0xe0);
						str += (char)(((ch & 0x0fc0) >> 6) | 0x80);
						str += (char)( (ch & 0x003f) | 0x80);
					}
				}
			}
		};

		template<typename Stream, typename ElementType>
		struct Reader<Stream, std::vector<ElementType> >
		{
			static void Read(Stream &s, std::vector<ElementType> &ref)
			{
				u32 size;
				s >> size;
				ref.clear();
				while(size--)
				{
					ElementType el;
					s >> el;
					ref.push_back(el);
				}
			}
		};
	}

	class Stream
	{
		const ByteArray &	_data;
		size_t				_offset;

	public:
		Stream(const ByteArray & data, size_t offset = 0): _data(data), _offset(offset) { }

		u8 ReadByte()
		{ return _data.at(_offset++); }

		const ByteArray & GetData() const
		{ return _data; }

		template<typename T>
		Stream& operator >> (T &ref)
		{
			impl::Reader<Stream, T>::Read(*this, ref);
			return *this;
		}
	};

}


#endif	/* STREAM_H */

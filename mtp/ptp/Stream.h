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
		struct Reader<Stream, std::string>
		{ static void Read(Stream &s, std::string &ref)
			{
				u8 len = s.ReadByte();
				ref.clear();
				while(len--)
				{
					u16 ch;
					Reader<Stream, u16>::Read(s, ch);
					ref += ((char)(ch & 0xff));
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

		template<typename T>
		Stream& operator >> (T &ref)
		{
			impl::Reader<Stream, T>::Read(*this, ref);
			return *this;
		}
	};

}


#endif	/* STREAM_H */

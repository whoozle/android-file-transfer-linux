#ifndef JOINEDOBJECTSTREAM_H
#define	JOINEDOBJECTSTREAM_H

#include <mtp/ptp/IObjectStream.h>

namespace mtp
{
	class JoinedObjectInputStreamBase : public IObjectInputStream
	{
	protected:
		bool					_stream1Exhausted;

		virtual IObjectInputStreamPtr GetStream1() const = 0;
		virtual IObjectInputStreamPtr GetStream2() const = 0;
		virtual void OnStream1Exhausted() { }

	public:
		JoinedObjectInputStreamBase(): _stream1Exhausted(false) { }

		virtual size_t Read(u8 *data, size_t size)
		{
			size_t r;
			if (!_stream1Exhausted)
			{
				r = GetStream1()->Read(data, size);
				if (r < size)
				{
					_stream1Exhausted = true;
					OnStream1Exhausted();
					r += GetStream2()->Read(data + r, size - r);
				}
			}
			else
				r = GetStream2()->Read(data, size);
			return r;
		}
	};

	class JoinedObjectInputStream : public JoinedObjectInputStreamBase
	{
		IObjectInputStreamPtr	_stream1, _stream2;
		size_t					_stream1Size, _stream2Size;

	private:
		virtual IObjectInputStreamPtr GetStream1() const
		{ return _stream1; }
		virtual IObjectInputStreamPtr GetStream2() const
		{ return _stream2; }

	public:
		JoinedObjectInputStream(IObjectInputStreamPtr s1, IObjectInputStreamPtr s2):
			_stream1(s1), _stream2(s2), _stream1Size(s1->GetSize()), _stream2Size(s2->GetSize())
		{ }

		virtual u64 GetSize() const
		{ return _stream1Size + _stream2Size; }

	};
	DECLARE_PTR(JoinedObjectInputStream);

	class JoinedObjectOutputStreamBase : public IObjectOutputStream
	{
	protected:
		bool					_stream1Exhausted;

		virtual IObjectOutputStreamPtr GetStream1() const = 0;
		virtual IObjectOutputStreamPtr GetStream2() const = 0;
		virtual void OnStream1Exhausted() { }

	public:
		JoinedObjectOutputStreamBase(): _stream1Exhausted(false) { }

		virtual size_t Write(const u8 *data, size_t size)
		{
			size_t r;
			if (!_stream1Exhausted)
			{
				r = GetStream1()->Write(data, size);
				if (r < size)
				{
					_stream1Exhausted = true;
					OnStream1Exhausted();
					r += GetStream2()->Write(data + r, size - r);
				}
			}
			else
				r = GetStream2()->Write(data, size);

			return r;
		}
	};

	class JoinedObjectOutputStream : public JoinedObjectOutputStreamBase
	{
		IObjectOutputStreamPtr	_stream1, _stream2;
		size_t					_offset;

	private:
		virtual IObjectOutputStreamPtr GetStream1() const
		{ return _stream1; }
		virtual IObjectOutputStreamPtr GetStream2() const
		{ return _stream2; }

	public:
		JoinedObjectOutputStream(IObjectOutputStreamPtr s1, IObjectOutputStreamPtr s2):
			_stream1(s1), _stream2(s2)
		{ }

	};
	DECLARE_PTR(JoinedObjectOutputStream);

}


#endif	/* JOINEDOBJECTSTREAM_H */

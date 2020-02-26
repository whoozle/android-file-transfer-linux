/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2020  Vladimir Menshakov

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef AFTL_MTP_PTP_JOINEDOBJECTSTREAM_H
#define AFTL_MTP_PTP_JOINEDOBJECTSTREAM_H

#include <mtp/ptp/IObjectStream.h>

namespace mtp
{
	class JoinedObjectInputStreamBase : public IObjectInputStream, public CancellableStream
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
			CheckCancelled();
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

	class JoinedObjectInputStream final: public JoinedObjectInputStreamBase  //! Input stream made of two subsequent stream
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

	class JoinedObjectOutputStreamBase : public IObjectOutputStream, public CancellableStream
	{
	protected:
		bool					_stream1Exhausted;

		virtual IObjectOutputStreamPtr GetStream1() const = 0;
		virtual IObjectOutputStreamPtr GetStream2() const = 0;
		virtual void OnStream1Exhausted() { }

	public:
		JoinedObjectOutputStreamBase(): _stream1Exhausted(false) { }

		size_t Write(const u8 *data, size_t size) override
		{
			CheckCancelled();
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

	class JoinedObjectOutputStream final: public JoinedObjectOutputStreamBase //! Output stream made of two subsequent stream
	{
		IObjectOutputStreamPtr	_stream1, _stream2;

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

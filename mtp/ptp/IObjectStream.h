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

#ifndef AFTL_MTP_PTP_IOBJECTSTREAM_H
#define AFTL_MTP_PTP_IOBJECTSTREAM_H

#include <mtp/types.h>
#include <atomic>
#include <stdexcept>

namespace mtp
{
	struct ICancellableStream //! interfaces for streams with cancellation functionality
	{
		virtual ~ICancellableStream() = default;
		virtual void Cancel() = 0;
	};
	DECLARE_PTR(ICancellableStream);

	struct OperationCancelledException : public std::runtime_error //! exception thrown if stream was cancelled
	{
		OperationCancelledException(): std::runtime_error("operation cancelled") { }
	};

	class CancellableStream : public virtual ICancellableStream //! default implementation of \ref ICancellableStream
	{
		std::atomic_bool _cancelled;

	public:
		CancellableStream(): _cancelled(false)
		{ }

		void Cancel()
		{ _cancelled.store(true); }

		void CheckCancelled()
		{
			bool cancelled = _cancelled.load();
			if (cancelled)
				throw OperationCancelledException();
		}
	};

	struct IObjectInputStream : public virtual ICancellableStream //! Basic input stream interface
	{
		virtual u64 GetSize() const = 0;
		virtual size_t Read(u8 *data, size_t size) = 0;
	};
	DECLARE_PTR(IObjectInputStream);

	struct IObjectOutputStream : public virtual ICancellableStream //! Basic output stream interface
	{
		virtual size_t Write(const u8 *data, size_t size) = 0;
	};
	DECLARE_PTR(IObjectOutputStream);

}

#endif	/* IOBJECTSTREAM_H */

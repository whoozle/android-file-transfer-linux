#ifndef IOBJECTSTREAM_H
#define	IOBJECTSTREAM_H

#include <mtp/types.h>

namespace mtp
{

	struct IObjectInputStream
	{
		virtual ~IObjectInputStream() { }
		virtual u64 GetSize() const = 0;
		virtual size_t Read(u8 *data, size_t size) = 0;
	};
	DECLARE_PTR(IObjectInputStream);

	struct IObjectOutputStream
	{
		virtual ~IObjectOutputStream() { }
		virtual size_t Write(const u8 *data, size_t size) = 0;
	};
	DECLARE_PTR(IObjectOutputStream);

}

#endif	/* IOBJECTSTREAM_H */

#ifndef AFT_BACKEND_LINUX_USB_BUFFERALLOCATOR_H
#define AFT_BACKEND_LINUX_USB_BUFFERALLOCATOR_H

#include <mtp/ByteArray.h>
#include <mtp/types.h>
#include <sys/mman.h>

namespace mtp { namespace usb
{
	struct IBuffer
	{
		virtual ~IBuffer() { }

		virtual u8 * GetData() = 0;
		virtual size_t GetSize() = 0;
	};
	DECLARE_PTR(IBuffer);

	class ByteArrayBuffer : private ByteArray, public IBuffer
	{
	public:
		ByteArrayBuffer(size_t size): ByteArray(size)
		{ }

		virtual u8 * GetData() override
		{ return data(); }
		virtual size_t GetSize() override
		{ return size(); }
	};

	class BufferAllocator
	{
		int _fd;

	public:
		BufferAllocator(int fd)
		{ }

		IBufferPtr Allocate(size_t size)
		{
			return std::make_shared<ByteArrayBuffer>(size);
		}
	};

}}

#endif

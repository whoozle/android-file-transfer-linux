#ifndef AFT_BACKEND_LINUX_USB_BUFFERALLOCATOR_H
#define AFT_BACKEND_LINUX_USB_BUFFERALLOCATOR_H

#include <mtp/ByteArray.h>
#include <mtp/types.h>
#include <mtp/log.h>
#include <Exception.h>
#include <sys/mman.h>
#include <unistd.h>

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

		~ByteArrayBuffer()
		{ }

		virtual u8 * GetData() override
		{ return data(); }
		virtual size_t GetSize() override
		{ return size(); }
	};

	class MappedBuffer : public IBuffer
	{
		u8 *		_data;
		size_t		_size;

	public:
		MappedBuffer(u8 *data, size_t size): _data(data), _size(size)
		{ }
		virtual u8 * GetData() override
		{ return _data; }
		virtual size_t GetSize() override
		{ return _size; }
	};
	using MappedBufferWeakPtr = std::weak_ptr<MappedBuffer>;
	DECLARE_PTR(MappedBuffer);

	class BufferAllocator
	{
		static constexpr size_t Buffers = 2;

		int			_fd;
		long		_pageSize;
		u8 *		_buffer;
		size_t		_bufferSize;

		std::array<MappedBufferWeakPtr, Buffers> _buffers;

	public:
		BufferAllocator(int fd): _pageSize(sysconf(_SC_PAGESIZE)), _buffer(nullptr), _bufferSize(0)
		{
			if (_pageSize <= 0)
				throw posix::Exception("sysconf(_SC_PAGESIZE)");
			debug("page size = ", _pageSize);
		}

		IBufferPtr Allocate(size_t size)
		{
//			if (_fd < 0)
				return std::make_shared<ByteArrayBuffer>(size);

			if (!_buffer)
			{
				_bufferSize = (size + _pageSize - 1) / _pageSize * _pageSize;
				_buffer = static_cast<u8 *>(mmap(NULL, _bufferSize * Buffers, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0));
				if (_buffer == MAP_FAILED)
					throw posix::Exception("mmap failed");
				debug("mapped buffer of ", _bufferSize * Buffers, " bytes to", static_cast<void *>(_buffer));
			}
			for(size_t i = 0; i < _buffers.size(); ++i)
			{
				IBufferPtr ptr = _buffers[i].lock();
				if (!ptr)
				{
					auto buffer = std::make_shared<MappedBuffer>(_buffer + i * _bufferSize, size);
					_buffers[i] = buffer;
					return buffer;
				}
			}
			throw std::runtime_error("BufferAllocator::Allocate: out of mapped memory");
		}
	};

}}

#endif

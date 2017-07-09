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
	class BufferAllocator;
	class Buffer
	{
		friend class BufferAllocator;

		BufferAllocator *	_parent;
		u8 *				_data;
		size_t				_size;

	public:
		Buffer(BufferAllocator *parent, u8 *data, size_t size): _parent(parent), _data(data), _size(size)
		{ }
		~Buffer();

		u8 * GetData() const
		{ return _data; }
		size_t GetSize() const
		{ return _size; }
	};

	class BufferAllocator : Noncopyable
	{
		static constexpr size_t Buffers		= 16; //for parallel endpoint access
		static constexpr size_t BufferSize	= 64 * 1024;

		std::mutex	_mutex;
		int			_fd;
		long		_pageSize;
		u8 *		_buffer;
		size_t		_bufferSize;
		ByteArray	_normalBuffer;

		std::array<bool, Buffers> _bufferAllocated;

		void AllocateNormalBuffer()
		{
			_fd = -1;
			_normalBuffer.resize(Buffers * BufferSize);
			_buffer = _normalBuffer.data();
			_bufferSize = _normalBuffer.size();
		}

	public:
		BufferAllocator(int fd): _fd(fd), _pageSize(sysconf(_SC_PAGESIZE)), _buffer(nullptr), _bufferSize(0), _bufferAllocated()
		{
			if (_pageSize <= 0)
				throw posix::Exception("sysconf(_SC_PAGESIZE)");
			debug("page size = ", _pageSize);
		}

		~BufferAllocator()
		{
			if (_fd >= 0)
				munmap(_buffer, _bufferSize);
		}

		void Free(Buffer &buffer)
		{
			scoped_mutex_lock l(_mutex);
			size_t index = (buffer._data - _buffer) / BufferSize;
			_bufferAllocated.at(index) = false;
		}

		Buffer Allocate(size_t size)
		{
			scoped_mutex_lock l(_mutex);
			if (!_buffer)
			{
				_bufferSize = (BufferSize + _pageSize - 1) / _pageSize * _pageSize;
				if (_fd >= 0)
				{
					try
					{
						auto buffer = static_cast<u8 *>(mmap(nullptr, _bufferSize * Buffers, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0));
						if (buffer == MAP_FAILED)
							throw posix::Exception("mmap failed");

						_buffer = buffer;
						debug("mapped buffer of ", _bufferSize * Buffers, " bytes to ", static_cast<void *>(_buffer));
					}
					catch(const std::exception &ex)
					{
						error("zerocopy allocator failed: ", ex.what());
						AllocateNormalBuffer();
					}
				}
				else
					AllocateNormalBuffer();
			}
			if (size > BufferSize)
				size = BufferSize;

			for(size_t i = 0; i < _bufferAllocated.size(); ++i)
			{
				if (!_bufferAllocated[i])
				{
					_bufferAllocated[i] = true;
					return Buffer(this, _buffer + BufferSize * i, size);
				}
			}
			throw std::runtime_error("BufferAllocator::Allocate: out of mapped memory");
		}
	};

	Buffer::~Buffer()
	{
		_parent->Free(*this);
	}

}}

#endif

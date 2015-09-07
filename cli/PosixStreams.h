#ifndef AFT_CLI_POSIXSTREAMS_H
#define AFT_CLI_POSIXSTREAMS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <mtp/ptp/IObjectStream.h>

namespace
{

	class ObjectInputStream : public mtp::IObjectInputStream
	{
		int			_fd;
		mtp::u64	_size;

	public:
		ObjectInputStream(const std::string &fname) : _fd(open(fname.c_str(), O_RDONLY))
		{
			if (_fd < 0)
			{
				perror("open");
				throw std::runtime_error("cannot open file: " + fname);
			}
#ifdef MTP_USES_STAT64
			struct stat64 st;
			if (stat64(fname.c_str(), &st) != 0)
				throw std::runtime_error("stat failed");
			_size = st.st_size;
#else
			struct stat st;
			if (stat(fname.c_str(), &st) != 0)
				throw std::runtime_error("stat failed");
			_size = st.st_size;
#endif
		}

		~ObjectInputStream()
		{ close(_fd); }

		mtp::u64 GetSize() const
		{ return _size; }

		virtual size_t Read(mtp::u8 *data, size_t size)
		{
			ssize_t r = read(_fd, data, size);
			if (r < 0)
				throw std::runtime_error("read failed");
			return r;
		}
	};

	class ObjectOutputStream : public mtp::IObjectOutputStream
	{
		int		_fd;

	public:
		ObjectOutputStream(const std::string &fname) : _fd(open(fname.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644))
		{
			if (_fd < 0)
			{
				perror("open");
				throw std::runtime_error("cannot open file: " + fname);
			}
		}

		~ObjectOutputStream()
		{ close(_fd); }

		virtual size_t Write(const mtp::u8 *data, size_t size)
		{
			ssize_t r = write(_fd, data, size);
			if (r < 0)
				throw std::runtime_error("write failed");
			return r;
		}
	};

}

#endif

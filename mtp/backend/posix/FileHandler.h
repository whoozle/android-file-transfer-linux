#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include <mtp/types.h>

namespace mtp { namespace posix
{

	class FileHandler : Noncopyable
	{
		int _fd;

	public:
		FileHandler(int fd): _fd(fd) { }
		~FileHandler();

		int Get() const
		{ return _fd; }
	};

}}

#endif

/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2017  Vladimir Menshakov

    Android File Transfer For Linux is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    Android File Transfer For Linux is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Android File Transfer For Linux.
    If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef POSIX_FILEHANDLER_H
#define POSIX_FILEHANDLER_H

#include <mtp/types.h>
#include <poll.h>

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

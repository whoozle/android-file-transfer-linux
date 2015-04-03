/*
 * Android File Transfer for Linux: MTP client for android devices
 * Copyright (C) 2015  Vladimir Menshakov

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef DIRECTORY_H
#define	DIRECTORY_H

#include <usb/Exception.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>

namespace mtp { namespace usb
{
	class Directory : Noncopyable
	{
	private:
		DIR *				_dir;
		std::vector<u8>		_buffer;

	public:
		Directory(const std::string &path): _dir(opendir(path.c_str()))
		{
			if (!_dir)
				throw Exception("opendir");

			long name_max = pathconf(path.c_str(), _PC_NAME_MAX);
			if (name_max == -1)         /* Limit not defined, or error */
				name_max = 255;         /* Take a guess */

			_buffer.resize(offsetof(struct dirent, d_name) + name_max + 1);
		}

		std::string Read()
		{
			dirent *buffer = static_cast<dirent *>(static_cast<void *>(_buffer.data()));
			dirent *entry = 0;
			int r = readdir_r(_dir, buffer, &entry);
			if (r)
				throw Exception("readdir_r", r);
			return entry? entry->d_name: "";
		}

		~Directory()
		{ closedir(_dir); }

		static int ReadInt(const std::string &path)
		{
			FILE *f = fopen(path.c_str(), "rt");
			if (!f)
				throw Exception("opening " + path);
			int r = 0;
			fscanf(f, "%x", &r);
			fclose(f);
			return r;
		}
	};
}}

#endif	/* DIRECTORY_H */

/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015  Vladimir Menshakov

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

#ifndef DIRECTORY_H
#define	DIRECTORY_H

#include <mtp/ByteArray.h>
#include <Exception.h>
#include <vector>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>

namespace mtp { namespace usb
{
	class File : Noncopyable
	{
		FILE *_f;

	public:
		File(const std::string &path) : _f(fopen(path.c_str(), "rb"))
		{
			if (!_f)
				throw posix::Exception("open " + path);
		}

		~File()
		{ fclose(_f); }

		FILE *GetHandle()
		{ return _f; }

		std::string ReadLine(size_t bufSize = 1024)
		{
			std::vector<char> buf(bufSize);
			if (!fgets(buf.data(), buf.size(), _f))
				throw posix::Exception("fgets");
			return buf.data();
		}

		ByteArray ReadAll()
		{
			static const size_t step = 4096;

			fseek(_f, 0, SEEK_SET);
			ByteArray buf;
			size_t r;
			do
			{
				size_t offset = buf.size();
				buf.resize(offset + step);
				r = fread(buf.data() + offset, 1, step, _f);
			}
			while(r == step);
			buf.resize(buf.size() - step + r);
			return buf;
		}

		int ReadInt(int base)
		{
			int r;
			switch(base)
			{
				case 16: if (fscanf(_f, "%x", &r) != 1) throw std::runtime_error("cannot read number"); break;
				case 10: if (fscanf(_f, "%d", &r) != 1) throw std::runtime_error("cannot read number"); break;
				default: throw std::runtime_error("invalid base");
			}
			return r;
		}
	};

	class Directory : Noncopyable
	{
	private:
		DIR *				_dir;
		std::vector<u8>		_buffer;

	public:
		Directory(const std::string &path): _dir(opendir(path.c_str()))
		{
			if (!_dir)
				throw posix::Exception("opendir");

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
				throw posix::Exception("readdir_r", r);
			return entry? entry->d_name: "";
		}

		~Directory()
		{ closedir(_dir); }

		static int ReadInt(const std::string &path, int base = 16)
		{
			File f(path);
			return f.ReadInt(base);
		}

		static std::string ReadString(const std::string &path)
		{
			File f(path);
			std::string str = f.ReadLine();
			size_t end = str.find_last_not_of(" \t\r\n\f");
			return end != str.npos? str.substr(0, end + 1): str;
		}

		static ByteArray ReadAll(const std::string &path)
		{
			File f(path);
			return f.ReadAll();
		}
	};
}}

#endif	/* DIRECTORY_H */

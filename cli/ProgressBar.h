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

#ifndef PROGRESSBAR_H
#define	PROGRESSBAR_H

#include <string>
#include <mtp/types.h>
#include <stdio.h>

namespace cli
{

	class ProgressBar
	{
		static const unsigned Junk = 9;

		std::string _title;
		int			_width;
		int			_maxWidth;

	public:
		ProgressBar(const std::string & title, int w, int max): _width(w)
		{
			_maxWidth = max - _width - Junk;
			if (_maxWidth < 1)
				throw std::runtime_error("insufficient space for progress bar");
			printf("config %d %d %d\n", _width, _maxWidth, max);

			int titleSize = mtp::OutputStream::Utf8Length(title);
			if (titleSize > _maxWidth)
			{
				int chompLeft = _maxWidth / 2; //utf unaware, fixme
				int chompRight = _maxWidth - chompLeft;
				_title = title.substr(0, chompLeft) + "…" + title.substr(title.size() - chompRight);
			}
			else
				_title = title;
		}

		void operator()(mtp::u64 current, mtp::u64 total)
		{
			unsigned percentage = current * 100 / total;
			printf("%3u%% [", percentage );
			unsigned width = current * _width / total;
			unsigned spaces = _width - width;
			while(width--)
				fputc('=', stdout);
			while(spaces--)
				fputc(' ', stdout);

			printf("] %s\n\033[1A\033[2K", _title.c_str());
		}
	};
}


#endif

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
		unsigned	_width;
		unsigned	_maxWidth;

	public:
		ProgressBar(const std::string & title, unsigned w, unsigned max): _title(title), _width(w), _maxWidth(max)
		{
			if (_maxWidth < Junk + _width)
			{
				if (_width < Junk)
					throw std::runtime_error("insufficient space for progress bar");
				_maxWidth = _width - Junk;
			}
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

			unsigned maxTitleSize = _maxWidth - Junk;
			unsigned titleSize = mtp::OutputStream::Utf8Length(_title);
			std::string title;
			if (titleSize > maxTitleSize)
			{
				unsigned chompLeft = maxTitleSize / 2; //utf unaware, fixme
				unsigned chompRight = maxTitleSize - chompLeft;
				title = _title.substr(0, chompLeft) + "…" + _title.substr(chompRight);
			}
			else
				title = _title;
			printf("] %s\n\033[1A\033[2K", title.c_str());
		}
	};
}


#endif

/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2020  Vladimir Menshakov

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef AFTL_CLI_PROGRESSBAR_H
#define AFTL_CLI_PROGRESSBAR_H

#include <string>
#include <mtp/types.h>
#include <mtp/log.h>
#include <stdio.h>

namespace cli
{
	class EventProgressBar
	{
		std::string		_title;
		int				_steps;
		mtp::u64		_current;

	public:
		EventProgressBar(const std::string &title, int steps = 1000): _title(title), _steps(steps), _current(0) { }

		void operator()(mtp::u64 offset, mtp::u64 total)
		{
			auto current = offset * _steps / total;
			if (current != _current || offset >= total)
			{
				_current = current;
				mtp::print(":progress ", _title, " ", offset, " ", total);
			}
		}

	};

	class ProgressBar
	{
		static const unsigned Junk = 9;

		std::string _title;
		int			_width;
		int			_maxWidth;
		unsigned	_percentage;

	public:
		ProgressBar(const std::string & title, int w, int max): _width(w), _percentage(-1)
		{
			_maxWidth = max - _width - Junk;
			if (_maxWidth < 1)
				throw std::runtime_error("insufficient space for progress bar");

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
			unsigned percentage = total? current * 100 / total: 100;
			if (_percentage != percentage)
			{
				_percentage = percentage;
				printf("%3u%% [", percentage );
				unsigned width = total? current * _width / total: _width;
				unsigned spaces = _width - width;
				while(width--)
					fputc('=', stdout);
				while(spaces--)
					fputc(' ', stdout);

				printf("] %s\n\033[1A\033[2K", _title.c_str());
			}
		}
	};
}


#endif

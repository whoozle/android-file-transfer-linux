/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2018  Vladimir Menshakov

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

#ifndef AFT_CLI_TOKENIZER_H
#define AFT_CLI_TOKENIZER_H

#include <string>
#include <sstream>
#include <cli/Tokens.h>

namespace cli
{

	class Tokenizer
	{
	public:
		const std::string &	_text;
		Tokens &			_tokens;
		size_t				_inputPosition;
		std::stringstream 	_tempString;

	public:
		Tokenizer(const std::string &text, Tokens &tokens);

		void Write(const char *text, size_t size)
		{ _tempString.write(text, size); }
		void Write(const char ch)
		{ _tempString << ch; }
		void NextArgument();
		size_t Input(char *buf, size_t max_size);
	};
}

#endif


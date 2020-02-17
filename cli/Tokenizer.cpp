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

#include <cli/Tokenizer.h>
#include <cli/arg_lexer.l.h>
#include <algorithm>

namespace cli
{
	Tokenizer::Tokenizer(const std::string &text, Tokens &tokens): _text(text), _tokens(tokens), _inputPosition(0)
	{
		void *scanner;
		args_lex_init_extra(this, &scanner);
		//token_set_debug(1, scanner);
		args_lex(scanner);
		args_lex_destroy(scanner); //not exception safe
		NextArgument();
	}

	void Tokenizer::NextArgument()
	{
		std::string arg = _tempString.str();
		if (!arg.empty())
		{
			_tokens.push_back(arg);
			_tempString.str(std::string());
		}
	}

	size_t Tokenizer::Input(char *buf, size_t max_size)
	{
		size_t n = std::min(max_size, _text.size() - _inputPosition);
		std::copy_n(_text.begin() + _inputPosition, n, buf);
		_inputPosition += n;
		return n;
	}
}

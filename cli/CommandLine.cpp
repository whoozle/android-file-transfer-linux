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

#include <cli/CommandLine.h>
#include <mtp/log.h>

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

namespace cli
{
	CommandLine::CommandLine()
	{
		rl_readline_name = const_cast<char *>("AFT");
		rl_attempted_completion_function = &CommandLine::CompletionCallback;
	}

	CommandLine & CommandLine::Get()
	{
		static CommandLine cmd;
		return cmd;
	}

	char * CommandLine::CompletionGenerator(const char *text, int state)
	{ return NULL; }

	char ** CommandLine::CompletionCallback(const char *text, int start, int end)
	{
		try
		{
			if (Get()._callback)
				return Get()._callback(text, start, end);
			else
				return NULL;
		}
		catch(const std::exception &ex)
		{ mtp::error(ex.what()); }
		return NULL;
	}

	bool CommandLine::ReadLine(const std::string &prompt, std::string &input)
	{
		char *line = readline(prompt.c_str());
		if (!line)
			return false;

		input.assign(line);
		add_history(input.c_str());
		return true;
	}

	bool CommandLine::ReadRawLine(std::string &input)
	{
		char buf[4096];
		char *r = fgets(buf, sizeof(buf), stdin);
		if (r)
			input.assign(r);
		else
			input.clear();
		return r;
	}

	std::string CommandLine::GetLineBuffer() const
	{ return rl_line_buffer; }
}

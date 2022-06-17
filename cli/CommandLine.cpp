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

#include <cli/CommandLine.h>
#include <mtp/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <readline.h>
#if HAVE_READLINE_HISTORY_H
#include <history.h>
#endif

namespace cli
{
	CommandLine::CommandLine()
	{
		rl_readline_name = const_cast<char *>("AFT");
		rl_attempted_completion_function = &CommandLine::CompletionCallback;
		char *home = getenv("HOME");
		if (home)
		{
			_historyPath = std::string(home) + "/.config";
			mkdir(_historyPath.c_str(), 0700);
			_historyPath += "/whoozle.github.io";
			mkdir(_historyPath.c_str(), 0700);
			_historyPath += "/aft-linux-cli.history";
			read_history(_historyPath.c_str());
		}
	}

	CommandLine::~CommandLine()
	{
		write_history(_historyPath.c_str());
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

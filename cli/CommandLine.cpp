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
#include <cli/CommandLine.h>

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

namespace cli
{
	CommandLine::CommandLine()
	{
		rl_readline_name = "AFT";
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
		if (Get()._callback)
			return Get()._callback(text, start, end);
		else
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

	std::string CommandLine::GetLineBuffer() const
	{ return rl_line_buffer; }
}

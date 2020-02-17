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

#include <stdio.h>

namespace cli
{
	CommandLine::CommandLine()
	{ }

	CommandLine::~CommandLine()
	{ }

	CommandLine & CommandLine::Get()
	{
		static CommandLine cmd;
		return cmd;
	}

	bool CommandLine::ReadLine(const std::string &prompt, std::string &input)
	{
		fputs(prompt.c_str(), stdout);
		fflush(stdout);
		return ReadRawLine(input);
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
	{ return std::string(); }
}

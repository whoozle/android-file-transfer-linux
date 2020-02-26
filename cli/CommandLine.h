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

#ifndef AFTL_CLI_COMMANDLINE_H
#define AFTL_CLI_COMMANDLINE_H

#include <string>
#include <functional>

namespace cli
{
	class CommandLine
	{
	public:
		typedef std::function<char ** (const char *, int, int)> Callback;

	protected:
		CommandLine();
		~CommandLine();

	private:
		static char ** CompletionCallback(const char *text, int start, int end);
		static char * CompletionGenerator(const char *text, int state);

		std::string _historyPath;
		Callback	_callback;

	public:
		static CommandLine & Get();

		bool ReadLine(const std::string &prompt, std::string &input);
		bool ReadRawLine(std::string &input);
		void SetCallback(const Callback &callback)
		{ _callback = callback; }
		std::string GetLineBuffer() const;
	};
}

#endif


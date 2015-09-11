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
#ifndef AFT_CLI_READLINE_H
#define AFT_CLI_READLINE_H

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

	private:
		static char ** CompletionCallback(const char *text, int start, int end);
		static char * CompletionGenerator(const char *text, int state);

		Callback _callback;

	public:
		static CommandLine & Get();

		bool ReadLine(const std::string &prompt, std::string &input);
		void SetCallback(const Callback &callback)
		{ _callback = callback; }
		std::string GetLineBuffer() const;
	};
}

#endif


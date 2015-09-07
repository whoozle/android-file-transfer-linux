#include <cli/CommandLine.h>

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

namespace cli
{
	CommandLine::CommandLine()
	{ }

	bool CommandLine::ReadLine(const std::string &prompt, std::string &input)
	{
		char *line = readline(prompt.c_str());
		if (!line)
			return false;

		input.assign(line);
		return true;
	}
}

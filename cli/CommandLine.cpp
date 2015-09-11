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

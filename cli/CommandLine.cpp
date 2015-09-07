#include <cli/CommandLine.h>

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

namespace cli
{
	namespace
	{
		char * dupstr (const char * s)
		{
			char *r = strdup(s);
			if (!r)
				std::terminate();
			return r;
		}
	}

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
	{
		return state == 0? dupstr("dummy"): NULL;
	}

	char ** CommandLine::CompletionCallback(const char *text, int start, int end)
	{
		if (start == 0)
			return rl_completion_matches(text, &CompletionGenerator);
		else
			return NULL;
	}

	bool CommandLine::ReadLine(const std::string &prompt, std::string &input)
	{
		char *line = readline(prompt.c_str());
		if (!line)
			return false;

		input.assign(line);
		return true;
	}
}

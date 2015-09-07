#ifndef AFT_CLI_READLINE_H
#define AFT_CLI_READLINE_H

#include <string>

namespace cli
{
	class CommandLine
	{
	protected:
		CommandLine();

	private:
		static char ** CompletionCallback(const char *text, int start, int end);
		static char * CompletionGenerator(const char *text, int state);

	public:
		static CommandLine & Get();

		bool ReadLine(const std::string &prompt, std::string &input);
	};
}

#endif


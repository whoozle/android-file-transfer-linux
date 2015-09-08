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
	};
}

#endif


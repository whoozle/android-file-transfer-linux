#ifndef AFT_CLI_READLINE_H
#define AFT_CLI_READLINE_H

#include <string>

namespace cli
{
	class CommandLine
	{
	public:
		CommandLine();

		bool ReadLine(const std::string &prompt, std::string &input);
	};
}

#endif


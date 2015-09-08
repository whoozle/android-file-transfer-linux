#ifndef AFT_CLI_TOKENIZER_H
#define AFT_CLI_TOKENIZER_H

#include <string>
#include <sstream>
#include <cli/Tokens.h>

namespace cli
{

	class Tokenizer
	{
	public:
		const std::string &	_text;
		Tokens &			_tokens;
		size_t				_inputPosition;
		std::stringstream 	_tempString;

	public:
		Tokenizer(const std::string &text, Tokens &tokens);

		void Write(const char *text, size_t size)
		{ _tempString.write(text, size); }
		void NextArgument();
		size_t Input(char *buf, size_t max_size);
	};
}

#endif


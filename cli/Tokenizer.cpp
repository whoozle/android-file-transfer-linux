#include <cli/Tokenizer.h>
#include <cli/arg_lexer.l.h>
#include <algorithm>

namespace cli
{
	Tokenizer::Tokenizer(const std::string &text, Tokens &tokens): _text(text), _tokens(tokens), _inputPosition(0)
	{
		void *scanner;
		args_lex_init_extra(this, &scanner);
		//token_set_debug(1, scanner);
		args_lex(scanner);
		args_lex_destroy(scanner); //not exception safe
		NextArgument();
	}

	void Tokenizer::NextArgument()
	{
		std::string arg = _tempString.str();
		if (!arg.empty())
		{
			_tokens.push_back(arg);
			_tempString.str(std::string());
		}
	}

	size_t Tokenizer::Input(char *buf, size_t max_size)
	{
		size_t n = std::min(max_size, _text.size() - _inputPosition);
		std::copy_n(_text.begin() + _inputPosition, n, buf);
		_inputPosition += n;
		return n;
	}
}

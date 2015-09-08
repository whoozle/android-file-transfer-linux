#ifndef AFT_CLI_COMMAND_H
#define AFT_CLI_COMMAND_H

#include <cli/Tokens.h>

#include <mtp/types.h>

namespace cli
{

	struct ICommand
	{
		virtual ~ICommand() { }
		virtual void Execute(const Tokens &tokens) = 0;
	};
	DECLARE_PTR(ICommand);

	template<typename Func>
	struct Command : public ICommand
	{
		Func _func;
		Command(Func && func) : _func(func) { }

		virtual void Execute(const Tokens &tokens)
		{ }
	};


}

#endif


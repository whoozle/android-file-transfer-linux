#ifndef AFT_CLI_COMMAND_H
#define AFT_CLI_COMMAND_H

#include <cli/Tokens.h>
#include <mtp/types.h>
#include <mtp/function_invoker.h>
#include <mtp/make_tuple.h>
#include <functional>

namespace cli
{
	struct ICommand
	{
		virtual ~ICommand() { }
		virtual void Execute(const Tokens &tokens) const = 0;
	};
	DECLARE_PTR(ICommand);

	template<typename ... Args>
	struct Command : public ICommand
	{
		typedef std::function<void (Args...)> FuncType;

		FuncType _func;

		Command(FuncType && func) : _func(func) { }

		template<typename ...FuncArgs>
		static void Execute(std::function<void (FuncArgs...)> func, const Tokens & tokens)
		{
			auto args = mtp::make_tuple<Tokens::const_iterator, FuncArgs...>(tokens.begin(), tokens.end());
			mtp::invoke(func, args);
		}

		virtual void Execute(const Tokens &tokens) const
		{
			Execute(_func, tokens);
		}
	};


}

#endif


#ifndef AFT_CLI_COMMAND_H
#define AFT_CLI_COMMAND_H

#include <cli/Tokens.h>
#include <mtp/types.h>
#include <mtp/function_invoker.h>
#include <functional>

namespace cli
{
	namespace impl
	{

		template<typename ... Tail>
		struct TupleBuilder
		{
			std::tuple<> Values;
			TupleBuilder(Tokens::const_iterator tokens) { }
		};

		template<typename First, typename ... Tail>
		struct TupleBuilder<First, Tail...>
		{
			std::string						_value;
			TupleBuilder<Tail...> 			_next;
			std::tuple<First, Tail...> 		Values;

			TupleBuilder(Tokens::const_iterator tokens): _value(*tokens++), _next(tokens)
			{
				std::stringstream ss(_value);
				First value;
				ss >> value;
				Values = std::tuple_cat(std::make_tuple(value), _next.Values);
			}
		};

		template<typename ...Args>
		struct Executor
		{
			static void Execute(std::function<void (Args...)> func, Tokens::const_iterator tokens)
			{
				TupleBuilder<Args...> b(tokens);
				mtp::invoke(func, b.Values);
			}
		};
	}

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

		virtual void Execute(const Tokens &tokens) const
		{
			impl::Executor<Args...>::Execute(_func, tokens.begin());
		}
	};


}

#endif


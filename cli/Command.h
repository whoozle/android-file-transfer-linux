#ifndef AFT_CLI_COMMAND_H
#define AFT_CLI_COMMAND_H

#include <cli/Tokens.h>
#include <mtp/types.h>
#include <mtp/function_invoker.h>
#include <functional>
#include <sstream>

namespace cli
{
	namespace impl
	{

		template<typename ... Tail>
		struct TupleBuilder
		{
			std::tuple<> Values;
			TupleBuilder(Tokens::const_iterator begin, Tokens::const_iterator end) { }
		};

		template<typename First, typename ... Tail>
		struct TupleBuilder<First, Tail...>
		{
			typedef typename std::decay<First>::type	ValueType;

			std::string									_text;
			TupleBuilder<Tail...> 						_next;

			std::tuple<ValueType, Tail...>				Values;

			static Tokens::const_iterator Next(Tokens::const_iterator & begin, Tokens::const_iterator end)
			{
				if (begin == end)
					throw std::runtime_error("not enough arguments");
				return begin++;
			}
			
			TupleBuilder(Tokens::const_iterator begin, Tokens::const_iterator end): 
				_text(*Next(begin, end)), _next(begin, end)
			{
				if (begin == end)
					throw std::runtime_error("not enough argument for command");

				std::stringstream ss(_text);
				ValueType value;
				ss >> value;
				Values = std::tuple_cat(std::make_tuple(value), _next.Values);
			}
		};

		template<typename ...Args>
		struct Executor
		{
			static void Execute(std::function<void (Args...)> func, const Tokens & tokens)
			{
				TupleBuilder<Args...> b(tokens.begin(), tokens.end());
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
			impl::Executor<Args...>::Execute(_func, tokens);
		}
	};


}

#endif


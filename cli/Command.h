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

		template<typename IteratorType, typename ... Tail>
		struct TupleBuilder
		{
			std::tuple<> Values;
			TupleBuilder(IteratorType begin, IteratorType end) { }
		};

		template<typename IteratorType, typename First, typename ... Tail>
		struct TupleBuilder<IteratorType, First, Tail...>
		{
			typedef typename std::decay<First>::type	ValueType;

			std::string									_text;
			TupleBuilder<IteratorType, Tail...> 		_next;

			std::tuple<ValueType, Tail...>				Values;

			static IteratorType Next(IteratorType & begin, IteratorType end)
			{
				if (begin == end)
					throw std::runtime_error("not enough arguments");
				return begin++;
			}
			
			TupleBuilder(IteratorType begin, IteratorType end):
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

		template<typename ...FuncArgs>
		static void Execute(std::function<void (FuncArgs...)> func, const Tokens & tokens)
		{
			impl::TupleBuilder<Tokens::const_iterator, FuncArgs...> b(tokens.begin(), tokens.end());
			mtp::invoke(func, b.Values);
		}

		virtual void Execute(const Tokens &tokens) const
		{
			Execute(_func, tokens);
		}
	};


}

#endif


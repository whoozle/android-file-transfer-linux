#ifndef MTP_MAKE_TUPLE_H
#define	MTP_MAKE_TUPLE_H

#include <sstream>

namespace mtp
{
	namespace impl
	{

		template<typename IteratorType, typename ... Tail>
		struct TupleBuilder
		{
			std::tuple<>	Result;
			TupleBuilder(IteratorType begin, IteratorType end) { }
		};

		template<typename IteratorType, typename First, typename ... Tail>
		struct TupleBuilder<IteratorType, First, Tail...>
		{
			typedef typename std::decay<First>::type	ValueType;

			std::string									_text;
			TupleBuilder<IteratorType, Tail...> 		_next;

			typedef std::tuple<ValueType, Tail...>		ResultType;
			ResultType									Result;

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
				Result = std::tuple_cat(std::make_tuple(value), _next.Result);
			}
		};
	}

	template<typename IteratorType, typename ...FuncArgs>
	auto make_tuple(IteratorType begin, IteratorType end) -> typename impl::TupleBuilder<IteratorType, FuncArgs...>::ResultType
	{
		return impl::TupleBuilder<IteratorType, FuncArgs...>::Result;
	}
};

#endif

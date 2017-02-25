/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2017  Vladimir Menshakov

    Android File Transfer For Linux is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    Android File Transfer For Linux is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Android File Transfer For Linux.
    If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MTP_MAKE_TUPLE_H
#define	MTP_MAKE_TUPLE_H

#include <sstream>
#include <functional>

#include <mtp/types.h>
#include <mtp/Demangle.h>

namespace cli
{
	namespace impl
	{
		template<typename T>
		struct ValueConverter;

		template<typename T>
		struct StringStreamConverter
		{
			static T Convert(const std::string &text)
			{
				std::stringstream ss(text);
				T value;
				ss >> std::noskipws >> value;
				return value;
			}
		};

		template<>
		struct ValueConverter<mtp::u32> : StringStreamConverter<mtp::u32>
		{ };

		template<>
		struct ValueConverter<std::string>
		{
			static std::string Convert(const std::string &text)
			{ return text; }
		};

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

			typedef std::tuple<ValueType, typename std::decay<Tail>::type ... >		ResultType;
			ResultType									Result;

			static IteratorType Next(IteratorType & begin, IteratorType end)
			{
				if (begin == end)
					throw std::runtime_error("not enough arguments");
				return begin++;
			}

			TupleBuilder(IteratorType begin, IteratorType end):
				_text(*Next(begin, end)), _next(begin, end)
			{ Result = std::tuple_cat(std::make_tuple(ValueConverter<ValueType>::Convert(_text)), _next.Result); }
		};
	}

	template<typename IteratorType>
	std::tuple<> MakeTuple(IteratorType, IteratorType)
	{ return std::tuple<>(); }

	template<typename IteratorType, typename ...FuncArgs>
	auto MakeTuple(IteratorType begin, IteratorType end) -> typename impl::TupleBuilder<IteratorType, FuncArgs...>::ResultType
	{
		impl::TupleBuilder<IteratorType, FuncArgs...> b(begin, end);
		return b.Result;
	}
};

#endif

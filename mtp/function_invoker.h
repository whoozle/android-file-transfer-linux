/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2016  Vladimir Menshakov

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

#ifndef MTP_FUNCTION_INVOKER_H
#define MTP_FUNCTION_INVOKER_H

namespace mtp
{

	namespace impl
	{
		template<int ...> struct seq {};

		template<int N, int ...S> struct gens : gens<N-1, N-1, S...> {};

		template<int ...S> struct gens<0, S...>{ typedef seq<S...> type; };

		template <typename R, typename ...Args>
		struct function_invoker
		{
			std::tuple<Args...> params;
			std::function<void(Args...)> func;

			void dispatch()
			{ return call(typename gens<sizeof...(Args)>::type()); }

			template<int ...S>
			void call(seq<S...>)
			{ return func(std::get<S>(params) ...); }
		};
	}

	template<typename R, typename ... Args, typename TupleType>
	void invoke(const std::function<R(Args...)> &func, const TupleType &tuple)
	{
		impl::function_invoker<R, Args...> invoker = { tuple, func };
		invoker.dispatch();
	}

}


#endif


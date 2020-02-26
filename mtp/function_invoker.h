/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2020  Vladimir Menshakov

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef AFTL_MTP_FUNCTION_INVOKER_H
#define AFTL_MTP_FUNCTION_INVOKER_H

#include <functional>

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
			std::function<R (Args...)> func;

			R dispatch()
			{ return call(typename gens<sizeof...(Args)>::type()); }

			template<int ...S>
			R call(seq<S...>)
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


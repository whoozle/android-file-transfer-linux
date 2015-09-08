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


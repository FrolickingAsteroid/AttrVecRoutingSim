#ifndef LEXICOGRAPHIC_ORDER_HPP
#define LEXICOGRAPHIC_ORDER_HPP

#include "routing-algebras/orders/comparison-vocab.hpp"

namespace RoutingAlgebras
{
template <class... Ords>
struct LexicographicOrder
{
	template <class Alg>
	Comp operator()(const Alg &l, const Alg &r) const
	{
		Comp result = Comp::Equal;

		// the lambda function is called with a different type I at each iteration
		// I is a compile-time constant that iterates through the algebras
		unroll_and<sizeof...(Ords)>([&](auto I) -> bool {
			// I is not known at compile time because the number of attributes is unknown at
			// compile tim
			constexpr std::size_t i = decltype(I)::value;

			using Ord = std::tuple_element_t<i, std::tuple<Ords...>>;
			Comp c    = Ord{}(std::get<i>(l), std::get<i>(r));

			if (c == Comp::Equal)
			{
				return true;
			}

			result = c;
			return false;
		});

		return result;
	}

    private:
	// As seen in https://towardsdev.com/cpp-variadic-template-unrolling-zero-cost-loops-8a870ae0458e
	template <typename F, std::size_t... Is>
	static constexpr bool unroll_and(F &&f, std::index_sequence<Is...>)
	{
		// Fold expression: expands to f(0); f(1); f(2); ... at compile time.
		// One call per index, one comparison per algebra in the tuple
		// Stop as soon as one of the elements is better than the other
		return (f(std::integral_constant<std::size_t, Is>{}) && ...);
	}

	template <std::size_t N, typename F>
	static constexpr bool unroll_and(F &&f)
	{
		return unroll_and(std::forward<F>(f), std::make_index_sequence<N>{});
	}
};
} // namespace RoutingAlgebras
#endif

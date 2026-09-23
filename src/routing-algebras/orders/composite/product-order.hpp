#ifndef PRODUCT_ORDER_HPP
#define PRODUCT_ORDER_HPP

#include <tuple>
#include <utility>

#include "routing-algebras/orders/comparison-vocab.hpp"

namespace RoutingAlgebras
{
template <class... Ords>
struct ProductOrder
{
	template <class Alg>
	Comp operator()(const Alg &l, const Alg &r) const
	{
		bool lR = cmp(l, r, std::index_sequence_for<Ords...>{}); // l < r
		bool rL = cmp(r, l, std::index_sequence_for<Ords...>{}); // r < l

		if (lR && rL)
		{
			return Comp::Equal;
		}
		if (lR)
			return Comp::Better;
		if (rL)
			return Comp::Worse;
		return Comp::Incomparable;
	}

    private:
	template <class Alg, std::size_t... I>
	static bool cmp(const Alg &l, const Alg &r, std::index_sequence<I...>)
	{
		// comparison per algebra in a variable sized tuple
		return (... &&
			BetterOrEqual(std::tuple_element_t<I, std::tuple<Ords...>>{}(std::get<I>(l), std::get<I>(r))));
	}
};
} // namespace RoutingAlgebras
#endif
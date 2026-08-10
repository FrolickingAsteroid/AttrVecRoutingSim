#ifndef LEXICOGRAPHIC_ORDER_HPP
#define LEXICOGRAPHIC_ORDER_HPP

#include "routing-algebras/orders/comparison-vocab.hpp"

namespace RoutingAlgebras
{
template <class OrdA, class OrdB>
struct LexicographicOrder
{
	template <class Attr>
	Comp operator()(const Attr &l, const Attr &r) const
	{
		OrdA ord_a;
		Comp resultA = ord_a(l.a, r.a);

		if (resultA != Comp::Equal)
		{
			return resultA;
		}

		// defer to B in case of tie
		OrdB ord_b;
		return ord_b(l.b, r.b);
	}
};
} // namespace RoutingAlgebras
#endif

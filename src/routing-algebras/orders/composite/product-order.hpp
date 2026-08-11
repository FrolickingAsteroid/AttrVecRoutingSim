#ifndef PRODUCT_ORDER_HPP
#define PRODUCT_ORDER_HPP

#include "routing-algebras/orders/comparison-vocab.hpp"

namespace RoutingAlgebras
{
template <class OrdA, class OrdB>
struct ProductOrder
{
	template <class Attr>
	Comp operator()(const Attr &l, const Attr &r) const
	{
		OrdA ord_a;
		OrdB ord_b;

		bool lR = BetterOrEqual(ord_a(l.a, r.a)) && BetterOrEqual(ord_b(l.b, r.b));
		bool rL = BetterOrEqual(ord_a(r.a, l.a)) && BetterOrEqual(ord_b(r.b, l.b));

		if (lR && rL)
		{
			return Comp::Equal;
		}
		if (lR)
		{
			return Comp::Better;
		}
		if (rL)
		{
			return Comp::Worse;
		}
		return Comp::Incomparable;
	}
};
} // namespace RoutingAlgebras
#endif

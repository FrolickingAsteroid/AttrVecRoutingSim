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

		bool lR = beq(ord_a(l.a, r.a)) && beq(ord_b(l.b, r.b));
		bool rL = beq(ord_a(r.a, l.a)) && beq(ord_b(r.b, l.b));

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

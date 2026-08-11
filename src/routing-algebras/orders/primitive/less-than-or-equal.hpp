#ifndef LEQ_ORDER_H
#define LEQ_ORDER_H

#include <climits>

#include "routing-algebras/orders/comparison-vocab.hpp"

namespace RoutingAlgebras
{
// less-than-or-equal.hpp
template <class A>
struct LessThanOrEqual
{
	static constexpr int null    = INT_MAX;
	static constexpr int neutral = 0;

	Comp operator()(const A &l, const A &r) const
	{
		if (l.getAttr() < r.getAttr())
			return Comp::Better;
		if (r.getAttr() < l.getAttr())
			return Comp::Worse;
		return Comp::Equal;
	}
};
using LEQ = LessThanOrEqual<L>;
} // namespace RoutingAlgebras

#endif

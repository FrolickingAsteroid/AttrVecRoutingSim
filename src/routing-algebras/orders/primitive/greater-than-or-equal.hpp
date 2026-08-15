#ifndef GEQ_ORDER_H
#define GEQ_ORDER_H

#include <climits>

#include "routing-algebras/orders/comparison-vocab.hpp"

namespace RoutingAlgebras
{
template <class A>
struct GreaterThanOrEqual
{
	static constexpr int null    = INT_MAX;
	static constexpr int neutral = 0;

	Comp operator()(const A l, const A r) const
	{
		if (l > r)
		{
			return Comp::Better;
		}
		if (l == r)
		{
			return Comp::Equal;
		}
		return Comp::Worse;
	}
};
} // namespace RoutingAlgebras

#endif

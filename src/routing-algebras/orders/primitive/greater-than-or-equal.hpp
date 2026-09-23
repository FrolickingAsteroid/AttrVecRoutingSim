#ifndef GEQ_ORDER_H
#define GEQ_ORDER_H

#include <climits>

#include "routing-algebras/orders/comparison-vocab.hpp"

namespace RoutingAlgebras
{
template <class A>
struct GreaterThanOrEqual
{
	static constexpr A null = std::numeric_limits<A>::min();
	static constexpr int neutral = std::numeric_limits<A>::max();

	Comp operator()(const A l, const A r) const
	{
		if (l > r)
		{
			return Comp::Better;
		}
		if (l < r)
		{
			return Comp::Worse;
		}
		return Comp::Equal;
	}
};

} // namespace RoutingAlgebras

#endif

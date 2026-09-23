#ifndef LEQ_ORDER_H
#define LEQ_ORDER_H

#include "routing-algebras/orders/comparison-vocab.hpp"

namespace RoutingAlgebras
{
template <class A>
struct LessThanOrEqual
{
	static constexpr A neutral = std::numeric_limits<A>::min();
	static constexpr A null    = std::numeric_limits<A>::max();

	Comp operator()(const A &l, const A &r) const
	{
		if (l < r)
		{
			return Comp::Better;
		}
		if (r < l)
		{
			return Comp::Worse;
		}
		return Comp::Equal;
	}
};
} // namespace RoutingAlgebras

#endif

#ifndef LEQ_ORDER_H
#define LEQ_ORDER_H

#include <climits>

#include "routing-algebras/orders/comparison-vocab.hpp"

namespace RoutingAlgebras
{
typedef struct LessThanOrEqual
{
	static constexpr int null    = INT_MAX;
	static constexpr int neutral = 0;

	Comp operator()(const int l, const int r) const
	{
		if (l < r)
		{
			return Comp::Better;
		}

		if (l == r)
		{
			return Comp::Equal;
		}
		return Comp::Worse;
	}
} LEQ;
} // namespace RoutingAlgebras

#endif

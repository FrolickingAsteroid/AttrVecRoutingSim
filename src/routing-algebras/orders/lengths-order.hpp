#ifndef LENGTH_ORDER_H
#define LENGTH_ORDER_H

#include <climits>

namespace RoutingAlgebras
{
typedef struct TotalOrderLength {
	static constexpr int null    = INT_MAX;
	static constexpr int neutral = 0;

	bool operator()(int l, int r) const
	{
		return (l <= r);
	}

	static int Neutral()
	{
		return neutral;
	}

	static int Null()
	{
		return null;
	}
} TOL;
}

#endif

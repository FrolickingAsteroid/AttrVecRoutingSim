#ifndef SHORTEST_PATH_ALGEBRA_HPP
#define SHORTEST_PATH_ALGEBRA_HPP

#include "routing-algebras/attributes/lengths.hpp"
#include "routing-algebras/extensions/addition.hpp"
#include "routing-algebras/orders/primitive/less-than-or-equal.hpp"

namespace RoutingAlgebras
{
struct ShortestPath
{
	using Attr = L;
	using Ext  = ADD;
	using Ord  = LEQ;

	static Attr Neutral()
	{
		return Ord::neutral;
	}
	static Attr Null()
	{
		return Ord::null;
	}
};

using SP = ShortestPath;
} // namespace RoutingAlgebras
#endif

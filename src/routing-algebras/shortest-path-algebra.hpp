#ifndef SHORTEST_PATH_ALGEBRA_HPP
#define SHORTEST_PATH_ALGEBRA_HPP

#include "routing-algebras/attributes/lengths.hpp"
#include "routing-algebras/extensions/addition.hpp"
#include "routing-algebras/orders/primitive/less-than-or-equal.hpp"

namespace RoutingAlgebras
{
struct ShortestPath
{
	using BaseAttr = Length;
	using BaseExt  = ADD;
	using BaseOrd  = LEQ;

	using Attr = BaseAttr;
	using Ext  = BaseExt;
	using Ord  = BaseOrd;

	static Attr Neutral()
	{
		return BaseOrd::neutral;
	}
	static Attr Null()
	{
		return BaseOrd::null;
	}
};

using SP = ShortestPath;
} // namespace RoutingAlgebras
#endif

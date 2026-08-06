#ifndef LENGTH_HPP
#define LENGTH_HPP

#include "routing-algebras/extensions/lengths-extension.hpp"
#include "routing-algebras/orders/lengths-order.hpp"

namespace RoutingAlgebras
{

struct Lengths
{
	using BaseAttr = int;
	using BaseExt  = LE;
	using BaseOrd  = TOL;

	using Attr = BaseAttr;
	using Ext  = BaseExt;
	using Ord  = BaseOrd;
};

using L = Lengths;
} // namespace RoutingAlgebras

#endif

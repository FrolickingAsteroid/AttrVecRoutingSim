#ifndef LENGTH_HPP
#define LENGTH_HPP

#include "routing-algebras/extensions/lengths-extension.hpp"
#include "routing-algebras/orders/lengths-order.hpp"

namespace RoutingAlgebras
{

typedef struct Lengths {
	using BaseAttr = int;
	using Ext = LE;
	using Ord = TOL;
} L;

}

#endif

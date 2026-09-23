#ifndef ROUTING_ALGEBRAS_HPP
#define ROUTING_ALGEBRAS_HPP

#include "routing-algebras/comp-algebra.hpp"
#include "routing-algebras/prim-algebra.hpp"
#include "routing-algebras/sequenced-algebra.hpp"

// attributes, extensions and orders
#include "routing-algebras/attributes/attributes.hpp"
#include "routing-algebras/extensions/extensions.hpp"
#include "routing-algebras/orders/orders.hpp"

namespace RoutingAlgebras
{

// ---------------------------------------------------------------------------- *
//
// Primitive algebras
//
// ---------------------------------------------------------------------------- *

using SHORTEST = PrimAlgebra<L, Addition<L>, LessThanOrEqual<L>>;
using WIDEST   = PrimAlgebra<W, Minimum<W>, GreaterThanOrEqual<W>>;

using WIDEST_SHORTEST = CompAlgebra<LexicographicOrder, SHORTEST, WIDEST>;

// ---------------------------------------------------------------------------- *
//
// Path algebras
//
// ---------------------------------------------------------------------------- *

// HOPS_SUFFIX compares hop count first, then colex order on
// path suffixes
using HOPS_SUFFIX = PrimAlgebra<Path<int>, Concatenation<int>, SuffixOrder<int>>;

// all paths of equal length remain incomparable
using EXPLICIT_PATH = PrimAlgebra<Path<int>, Concatenation<int>, PathEquality<int>>;

// ---------------------------------------------------------------------------- *
//
// Composite algebras
//
// ---------------------------------------------------------------------------- *

// Shortest path algebra, tie broken on hop count then colex order on path suffixes.
using LEX_LENGTH_HOPS_SUFFIX = CompAlgebra<LexicographicOrder, SHORTEST, HOPS_SUFFIX>;

// Product order on Widest shortest path algebra and lex on hop count and
//  colex order on path suffixes
using LEX_LENGTH_WIDTH_SUFFIX = CompAlgebra<ProductOrder, WIDEST_SHORTEST, HOPS_SUFFIX>;

// Product order on widths and the lexicographic product on lengths, hops and suffix
using PROD_WIDTH_LENGTH_HOPS_SUFFIX = CompAlgebra<ProductOrder, WIDEST, LEX_LENGTH_HOPS_SUFFIX>;

// shortest explicit path algebra, where any two distinct paths of
// equal length remain incomparable
using LEX_LENGTH_EXPLICIT = CompAlgebra<LexicographicOrder, SHORTEST, EXPLICIT_PATH>;

} // namespace RoutingAlgebras
#endif
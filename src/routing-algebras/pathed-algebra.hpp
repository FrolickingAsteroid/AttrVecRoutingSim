#ifndef PATH_LEX_ALGEBRA_HPP
#define PATH_LEX_ALGEBRA_HPP

#include "routing-algebras/attributes/pathed-attributes.hpp"
#include "routing-algebras/extensions/pathed-extension.hpp"
#include "routing-algebras/orders/composite/lexicographic-order.hpp"
#include "routing-algebras/orders/primitive/equality.hpp"

namespace RoutingAlgebras
{
template <class BaseAlgebra, class Node = int>
struct EqPathLex
{
	using BaseAttr = BaseAlgebra::Attr;
	using BaseExt  = BaseAlgebra::Ext;
	using BaseOrd  = BaseAlgebra::Ord;

	using Attr = PathAttr<BaseAttr, Node>;
	using Ext  = PathedExtension<BaseAttr, Node, BaseExt>;
	using Ord  = LexicographicOrder<BaseOrd, PathEquality<Node>>;

	static Attr Neutral()
	{
		return Attr{BaseAlgebra::Neutral(), Path<Node>{}};
	}

	static Attr Neutral(Node p)
	{
		return Attr{BaseAlgebra::Neutral(), {p}};
	}
	static Attr Null()
	{
		return Attr{BaseAlgebra::Null(), Path<Node>{}};
	}
};
} // namespace RoutingAlgebras
#endif
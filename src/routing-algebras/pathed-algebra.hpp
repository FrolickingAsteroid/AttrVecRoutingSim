#ifndef PATH_LEX_ALGEBRA_HPP
#define PATH_LEX_ALGEBRA_HPP

#include "routing-algebras/attributes/pathed-attributes.hpp"
#include "routing-algebras/extensions/pathed-extension.hpp"
#include "routing-algebras/orders/composite/lexicographic-order.hpp"

namespace RoutingAlgebras
{
template <class BaseAlgebra, template <class> class PathOrd, class Node = int>
struct PathLex
{
	using Base = BaseAlgebra;

	using Attr = PathAttr<typename Base::Attr, Node>;
	using Ext  = PathedExtension<typename Base::Attr, Node, typename Base::Ext>;
	using Ord  = LexicographicOrder<typename Base::Ord, PathOrd<Node>>;

	// P is either a full path or a single node, defaults to an empty vector
	// for now, overload, there are no variadic functions with default values, only workarounds
	template <class... Args>
	static Attr Neutral(Args &&...args)
	{
		return Attr{Base::Neutral(std::forward<Args>(args)...), Path<Node>{}};
	}

	template <class P, class... Args>
	static Attr Neutral(P p, Args &&...args)
	{
		return Attr{Base::Neutral(std::forward<Args>(args)...), p};
	}

	static Attr Null()
	{
		return Attr{Base::Null(), Path<Node>{}};
	}
};
} // namespace RoutingAlgebras
#endif
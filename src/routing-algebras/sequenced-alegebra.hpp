#ifndef SEQUENCED_ALGEBRA_H
#define SEQUENCED_ALGEBRA_H

#include "routing-algebras/attributes/sequenced-attributes.hpp"
#include "routing-algebras/extensions/sequenced-extension.hpp"
#include "routing-algebras/orders/composite/product-order.hpp"
#include "routing-algebras/orders/primitive/greater-than-or-equal.hpp"

namespace RoutingAlgebras
{
template <class BaseAlgebra>
struct Sequenced
{
	// Version numbers encapsulate a base algebra, which can either be primitive (ex.: shortest paths)
	// or composite (ex.: widest-shortest path).
	using Base = BaseAlgebra;

	using Attr = SeqAttr<typename Base::Attr>;
	using Ext  = SeqExtension<typename Base::Attr, typename Base::Ext>;
	using Ord  = ProductOrder<GEQ, typename Base::Ord>;

	template <class... Args>
	static Attr Neutral(int n, Args &&...args)
	{
		return Attr{n, Base::Neutral(std::forward<Args>(args)...)};
	}

	static Attr Null(int n = 0)
	{
		return Attr{n, Base::Null()};
	}
};
} // namespace RoutingAlgebras
#endif

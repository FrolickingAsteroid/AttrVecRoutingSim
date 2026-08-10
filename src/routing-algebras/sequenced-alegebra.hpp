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
	using Base     = BaseAlgebra;
	using BaseAttr = BaseAlgebra::Attr;
	using BaseExt  = BaseAlgebra::Ext;
	using BaseOrd  = BaseAlgebra::Ord;

	using Attr = SeqAttr<BaseAttr>;
	using Ext  = SeqExtension<BaseAttr, BaseExt>;
	using Ord  = ProductOrder<GEQ, BaseOrd>;

	static Attr Neutral()
	{
		return Attr{0, BaseOrd::neutral};
	}

	static Attr Neutral(int n)
	{
		return Attr{n, BaseOrd::neutral};
	}

	static Attr Null()
	{
		return Attr{0, BaseOrd::null};
	}

	static Attr Null(int n)
	{
		return Attr{n, BaseOrd::null};
	}
};
} // namespace RoutingAlgebras
#endif

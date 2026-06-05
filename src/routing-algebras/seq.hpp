#ifndef SEQUENCED_H
#define SEQUENCED_H

namespace RoutingAlgebras
{
template <class A> struct SeqAttr {
	int n{}; // Sequence number
	A a; // Base routing algebra attribute

	SeqAttr() = default;

	SeqAttr(A a_)
		: n(0)
		, a(a_)
	{
	}

	SeqAttr(int n_, A a_)
		: n(n_)
		, a(a_)
	{
	}

	bool operator==(const SeqAttr<A> &other) const
	{
		return n == other.n && a == other.a;
	}

	bool operator!=(const SeqAttr<A> &other) const
	{
		return !(*this == other);
	}
};

// forward declaration
template <class A, class ExtBase> struct SeqExtension;
template <class A, class OrdBase> struct SeqOrd;

template <class BaseAlgebra> struct Sequenced {
	using BaseAttr = BaseAlgebra::BaseAttr;

	using Attr = SeqAttr<BaseAttr>;
	using Ext = SeqExtension<BaseAttr, typename BaseAlgebra::Ext>;
	using Ord = SeqOrd<BaseAttr, typename BaseAlgebra::Ord>;
};
}

#endif

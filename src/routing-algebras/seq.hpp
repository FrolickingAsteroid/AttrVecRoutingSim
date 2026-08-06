#ifndef SEQUENCED_H
#define SEQUENCED_H

namespace RoutingAlgebras
{
template <class A> struct SeqAttr
{
	int n{}; // Sequence number
	A   a;   // Base routing algebra attribute

	SeqAttr() = default;

	SeqAttr(A a_)
	    : n(0)
	    , a(a_)
	{
	}

	SeqAttr(const int n_, A a_)
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

	bool operator==(const A &other) const
	{
		return a == other;
	}

	friend std::ostream &operator<<(std::ostream &os, const SeqAttr &a)
	{
		os << "(" << a.n << ", " << a.a << ")";
		return os;
	}
};

// forward declaration
template <class A, class ExtBase> struct SeqExtension;
template <class A, class OrdBase> struct SeqOrd;

template <class BaseAlgebra> struct Sequenced
{
	using BaseAttr = typename BaseAlgebra::BaseAttr;
	using BaseExt  = typename BaseAlgebra::BaseExt;
	using BaseOrd  = typename BaseAlgebra::BaseOrd;

	using Attr = SeqAttr<BaseAttr>;
	using Ext  = SeqExtension<BaseAttr, BaseExt>;
	using Ord  = SeqOrd<BaseAttr, BaseOrd>;
};
} // namespace RoutingAlgebras

#endif

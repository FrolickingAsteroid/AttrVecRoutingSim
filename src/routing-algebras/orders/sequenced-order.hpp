#ifndef SEQ_ORDER_H
#define SEQ_ORDER_H

#include "routing-algebras/seq.hpp"

namespace RoutingAlgebras
{
template <class A, class OrdBase>
struct SeqOrd
{
	int N = 0;

	SeqOrd() = default;

	explicit SeqOrd(std::size_t N_)
	    : N(static_cast<int>(N_))
	{
	}

	static SeqAttr<A> Neutral()
	{
		return SeqAttr<A>{0, OrdBase::neutral};
	}

	static SeqAttr<A> Neutral(int n)
	{
		return SeqAttr<A>{n, OrdBase::neutral};
	}

	static SeqAttr<A> Null()
	{
		return SeqAttr<A>{0, OrdBase::null};
	}

	static SeqAttr<A> Null(int n)
	{
		return SeqAttr<A>{n, OrdBase::null};
	}

	int operator()(const SeqAttr<A> &l, const SeqAttr<A> &r) const
	{
		OrdBase ord; // attribute base order

		// l dominates r by sequence number
		if (l.n > r.n + N)
		{
			return 1;
		}

		// l dominates r by attr order
		if (l.n >= r.n && ord(l.a, r.a) && l != r)
		{
			return 1;
		}

		return 0;
	}
};
} // namespace RoutingAlgebras
#endif

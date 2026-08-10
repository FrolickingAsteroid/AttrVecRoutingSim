#ifndef SEQUENCED_H
#define SEQUENCED_H

namespace RoutingAlgebras
{
template <class A>
struct SeqAttr
{
	int a{}; // Sequence number
	A   b;   // Base routing algebra attribute

	SeqAttr() = default;

	SeqAttr(A a_)
	    : a(0)
	    , b(a_)
	{
	}

	SeqAttr(const int n_, A a_)
	    : a(n_)
	    , b(a_)
	{
	}

	bool empty() const
	{
		return b.empty();
	}

	bool operator==(const SeqAttr<A> &other) const
	{
		return a == other.a && b == other.b;
	}

	bool operator!=(const SeqAttr<A> &other) const
	{
		return !(*this == other);
	}

	bool operator==(const A &other) const
	{
		return b == other;
	}

	friend std::ostream &operator<<(std::ostream &os, const SeqAttr &a)
	{
		os << "(" << a.a << ", " << a.b << ")";
		return os;
	}
};
} // namespace RoutingAlgebras

#endif

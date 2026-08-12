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

	// forwards extra arguments into A's constructor
	template <class... Args>
	SeqAttr(const int n_ = 0, Args &&...args)
	    : a(n_)
	    , b(std::forward<Args>(args)...)
	{
	}

	// ---------------------------------------------------------------------------- *
	//
	// Getters and Setters
	//
	// ---------------------------------------------------------------------------- *

	void setSeqNum(const int a_)
	{
		a = a_;
	}

	void setAttr(const A &b_)
	{
		b = b_;
	}

	const int &getSeqNum() const
	{
		return a;
	}

	const A &getAttr() const
	{
		return b;
	}

	bool empty() const
	{
		return b.empty();
	}

	bool operator==(const SeqAttr &other) const
	{
		return a == other.a && b == other.b;
	}

	bool operator!=(const SeqAttr &other) const
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

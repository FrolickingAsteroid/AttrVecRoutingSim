#ifndef PATHED_ATTRIBUTES_HPP
#define PATHED_ATTRIBUTES_HPP

#include <ostream>

#include "routing-algebras/attributes/paths.hpp"
namespace RoutingAlgebras
{
template <class A, class Node>
struct PathAttr
{
	A          a;
	Path<Node> b;

	PathAttr() = default;

	PathAttr(A a_, Path<Node> b_)
	    : a(a_)
	    , b({b_})
	{
	}

	PathAttr(A a_, Node b_)
	    : a(a_)
	    , b({b_})
	{
	}

	const Path<Node> &getPath() const
	{
		return b.getPath();
	}

	bool empty() const
	{
		return b.empty();
	}

	bool operator==(const PathAttr &other) const
	{
		return a == other.a && b == other.b;
	}
	bool operator!=(const PathAttr &other) const
	{
		return !(*this == other);
	}
	friend std::ostream &operator<<(std::ostream &os, const PathAttr &p)
	{
		return os << p.a << " " << p.b; // try this later
	}

	friend std::istream &operator>>(std::istream &is, PathAttr &attr)
	{
		A a;
		is >> a;
		attr = PathAttr{a, Path<Node>{}};
		return is;
	}
};
} // namespace RoutingAlgebras
#endif
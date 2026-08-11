#ifndef LENGTHS_HPP
#define LENGTHS_HPP

#include <istream>
namespace RoutingAlgebras
{
struct Lengths
{
	int a;

	Lengths() = default;

	Lengths(int a_)
	    : a(a_)
	{
	}

	void setAttr(const int a_)
	{
		a = a_;
	}

	const int &getAttr() const
	{
		return a;
	}

	bool operator==(const Lengths &other) const
	{
		return a == other.a;
	}
	bool operator!=(const Lengths &other) const
	{
		return !(*this == other);
	}

	friend std::istream &operator>>(std::istream &is, Lengths &l)
	{
		int a_;
		is >> a_;
		l.a = a_;
		return is;
	}

	friend std::ostream &operator<<(std::ostream &os, const Lengths &p)
	{
		return os << p.a;
	}
};

using L = Lengths;
} // namespace RoutingAlgebras
#endif
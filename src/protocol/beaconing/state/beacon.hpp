#ifndef BEACON_HPP
#define BEACON_HPP

#include <routing-algebras/sequenced-ra.hpp>
#include <vector>

namespace Protocol::Beaconing
{
template <class A> struct Beacon {
	A attribute;
	std::vector<int> path;

	static constexpr double expiration = 10.0; //(no idea)

	double timeout;

	Beacon() = default;

	Beacon(int e, A a)
		: attribute(a)
	{
		addAsEntry(e);
	}

	bool expired(double now)
	{
		return now > timeout;
	}

	void setTimeout(double time)
	{
		timeout = time + expiration;
	}

	void addAsEntry(const int &entry)
	{
		path.insert(path.begin(), entry);
	}

	A getAttr()
	{
		return attribute;
	}

	void addAttr(const A &attr)
	{
		attribute = attr;
	}

	bool operator==(const Beacon<A> &other) const
	{
		return attribute == other.attribute && path == other.path;
	}

	bool operator!=(const Beacon<A> &other) const
	{
		return !(*this == other);
	}
};
} // namespace Protocol::Beaconing

#endif

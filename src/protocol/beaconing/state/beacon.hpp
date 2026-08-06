#ifndef BEACON_HPP
#define BEACON_HPP

#include <iomanip>
#include <vector>

namespace Protocol::Beaconing
{
template <class A>
struct Beacon
{
	A                attribute;
	std::vector<int> path;

	static constexpr double expiration = 1000000000; //(no idea)

	double timeout;

	Beacon() = default;

	Beacon(int e, A a)
	    : attribute(a)
	{
		addAsEntry(e);
	}

	bool expired(const double now) const
	{
		return now > timeout;
	}

	void setTimeout(const double time)
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

	friend std::ostream &operator<<(std::ostream &os, const Beacon<A> &b)
	{
		auto old_flags     = os.flags();
		auto old_precision = os.precision();

		os << "(" << std::fixed << std::setprecision(2) << b.timeout << "; " << b.attribute << ")";

		os.flags(old_flags);
		os.precision(old_precision);

		return os;
	}
};
} // namespace Protocol::Beaconing

#endif

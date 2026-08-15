#ifndef BEACON_HPP
#define BEACON_HPP

#include <iomanip>
#include <vector>

namespace Protocol::Beaconing
{
template <class RA>
struct Beacon
{
	using A   = RA::Attr;
	using Ext = RA::Ext;
	using Ord = RA::Ord;

	A      attr; // (attr, path)
	double timeout;

	static constexpr double expiration = 1000000000; //(no idea)

	Beacon() = default;

	Beacon(A attr_)
	    : attr(std::move(attr_))
	{
	}

	Beacon(A attr_, const double time)
	    : attr(std::move(attr_))
	{
		this->setTimeout(time);
	}

	bool expired(const double now) const
	{
		return now > timeout;
	}

	void setTimeout(const double time)
	{
		timeout = time + expiration;
	}

	bool isInvalid() const
	{
		auto p = attr.getAttr().getPath();
		return p.empty();
	}

	A getAttr()
	{
		return attr;
	}

	bool operator==(const Beacon<A> &other) const
	{
		return attr == other.attr;
	}

	bool operator!=(const Beacon<A> &other) const
	{
		return !(*this == other);
	}

	friend Beacon operator+(const Beacon &p, const A &link)
	{
		Ext ext;

		auto extended = ext(p.getAttr(), link);

		return Beacon(extended);
	}

	friend std::ostream &operator<<(std::ostream &os, const Beacon &p)
	{
		return os << p.attr;
	}

	friend std::ostream &operator<<(std::ostream &os, const std::vector<Beacon> &v)
	{

		for (std::size_t i = 0; i < v.size(); ++i)
		{
			os << v[i];
			if (i + 1 < v.size())
				os << "; ";
		}
		return os;
	}
};
} // namespace Protocol::Beaconing

#endif

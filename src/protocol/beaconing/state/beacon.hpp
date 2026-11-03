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

	static constexpr double expiration = 100;

	Beacon() = default;

	Beacon(A attr_)
	    : attr(std::move(attr_))
	{
	}

	Beacon(A attr_, const double time)
	    : attr(std::move(attr_))
	{
		this->updateTimeout(time);
	}

	bool expired(const double now) const
	{
		return now > timeout;
	}

	void setTimeout(const double time)
	{
		timeout = time;
	}

	void updateTimeout(const double time)
	{
		timeout = time + expiration;
	}

	double getTimeout() const
	{
		return timeout;
	}

	bool isInvalid() const
	{
		return attr == RA::Null();
	}

	static Beacon Invalid()
	{
		return Beacon(RA::Null());
	}
	A getAttr() const
	{
		return attr;
	}

	bool operator==(const Beacon &other) const
	{
		return attr == other.attr;
	}

	bool operator!=(const Beacon &other) const
	{
		return !(*this == other);
	}

	friend Beacon operator+(const Beacon &p, const A &link)
	{
		const auto &as_path = p.getAttr().getPath();
		const auto &newHop  = link.getAttr().getPath();

		// Paths containing loops are invalidated
		if (std::find(as_path.path.begin(), as_path.path.end(), newHop[0]) != as_path.path.end())
		{
			return Beacon::Invalid();
		}

		Ext ext;

		auto   extended = ext(p.getAttr(), link);
		Beacon p_(extended);
		p_.setTimeout(p.getTimeout());

		return p_;
	}

	friend std::ostream &operator<<(std::ostream &os, const Beacon &p)
	{
		return os << p.getTimeout() << ": " << p.attr;
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

#ifndef BGP_ATTRIBUTE_HPP
#define BGP_ATTRIBUTE_HPP

#include <algorithm>
#include <iostream>
#include <vector>

namespace Protocol::BGP
{
static constexpr int MaxASPathLength = 35;

template <class RA>
struct BGPAttribute
{
	using A   = RA::Attr;
	using Ext = RA::Ext;
	using Ord = RA::Ord;

	A                attribute;
	std::vector<int> as_path;

	BGPAttribute() = default;

	BGPAttribute(int asn, A attr)
	    : attribute(attr)
	    , as_path({asn})
	{
	}

	BGPAttribute(std::vector<int> path, A attr)
	    : attribute(attr)
	    , as_path(std::move(path))
	{
	}

	static BGPAttribute Neutral(int asn)
	{
		return BGPAttribute(asn, RA::Neutral());
	}

	static BGPAttribute Invalid()
	{
		return BGPAttribute(std::vector<int>{}, RA::Null());
	}

	int length() const
	{
		return static_cast<int>(as_path.size());
	}

	bool isInvalid() const
	{
		return as_path.empty();
	}

	A getAttr() const
	{
		return attribute;
	}

	void addAsEntry(int asn)
	{
		as_path.insert(as_path.begin(), asn);
	}

	void addAttr(const A &attr)
	{
		attribute = attr;
	}

	bool operator==(const BGPAttribute &other) const
	{
		return attribute == other.attribute;
	}

	bool operator!=(const BGPAttribute &other) const
	{
		return attribute != other.attribute || as_path != other.as_path;
	}

	bool operator<(const BGPAttribute &other) const
	{
		return attribute < other.attribute;

		// Preorder by AS path length as tiebreaker:
		// return attribute < other.attribute ||
		//        (attribute == other.attribute &&
		//         as_path.size() < other.as_path.size());

		// Total order, lexicographic tiebreak on second-to-last hop:
		// return attribute < other.attribute ||
		//        (attribute == other.attribute &&
		//         as_path.size() < other.as_path.size()) ||
		//        (attribute == other.attribute &&
		//         as_path.size() == other.as_path.size() &&
		//         as_path.size() > 1 &&
		//         as_path[as_path.size() - 2] < other.as_path[other.as_path.size() - 2]);
	}

	friend BGPAttribute operator+(const BGPAttribute &link, const BGPAttribute &a)
	{
		Ext ext;

		if (a.isInvalid())
		{
			return BGPAttribute::Invalid();
		}

		// the tail AS is already in the path (loop detection).
		if (std::find(a.as_path.begin(), a.as_path.end(), link.as_path[0]) != a.as_path.end())
		{
			return BGPAttribute::Invalid();
		}

		std::vector<int> new_path(a.as_path);
		new_path.push_back(link.as_path[0]);

		return BGPAttribute(std::move(new_path), ext(link.attribute, a.attribute));
	}

	friend std::ostream &operator<<(std::ostream &os, const BGPAttribute &a)
	{
		auto old_flags     = os.flags();
		auto old_precision = os.precision();

		os << a.attribute;

		os.flags(old_flags);
		os.precision(old_precision);

		return os;
	}

	friend std::ostream &operator<<(std::ostream &os, const std::vector<BGPAttribute> &v)
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
} // namespace Protocol::BGP

#endif
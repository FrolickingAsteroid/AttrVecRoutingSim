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

	A attribute;

	BGPAttribute() = default;

	explicit BGPAttribute(A attr)
	    : attribute(std::move(attr))
	{
	}

	static BGPAttribute Neutral(int asn)
	{
		return BGPAttribute(RA::Neutral(asn));
	}

	static BGPAttribute Invalid()
	{
		return BGPAttribute(RA::Null());
	}

	int length() const
	{
		return static_cast<int>(attribute.getPath().size());
	}

	bool isInvalid() const
	{
		return attribute == RA::Null();
	}

	A getAttr() const
	{
		return attribute;
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
		return !(*this == other);
	}


	friend BGPAttribute operator+(const BGPAttribute &link, const BGPAttribute &a)
	{
		if (a.isInvalid())
		{
			return BGPAttribute::Invalid();
		}

		const auto &as_path = a.attribute.getPath();
		const auto &newHop  = link.attribute.getPath();

		if (std::find(as_path.path.begin(), as_path.path.end(), newHop[0]) != as_path.path.end())
		{
			return BGPAttribute::Invalid();
		}

		Ext ext;

		return BGPAttribute(ext(a.attribute, link.attribute));
	}

	friend std::ostream &operator<<(std::ostream &os, const BGPAttribute &a)
	{
		return os << a.attribute;
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
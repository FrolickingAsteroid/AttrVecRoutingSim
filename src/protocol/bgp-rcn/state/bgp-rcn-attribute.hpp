#ifndef RCN_ATTRIBUTE_HPP
#define RCN_ATTRIBUTE_HPP

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "protocol/bgp/state/bgp-attribute.hpp"

namespace Protocol::RCN
{
template <class RA>
struct RCNAttribute : BGP::BGPAttribute<RA>
{
	using base = BGP::BGPAttribute<RA>;

	std::unordered_map<int, int> ts; // ts(u) for u in as_path

	RCNAttribute() = default;

	RCNAttribute(int asn, base::A attr, const int seq)
	    : base(asn, attr)
	{
		ts[asn] = seq;
	}

	RCNAttribute(std::vector<int> path, base::A attr, std::unordered_map<int, int> ts_)
	    : base(std::move(path), attr)
	    , ts(std::move(ts_))
	{
	}

	static RCNAttribute Invalid()
	{
		return RCNAttribute(std::vector<int>{}, RA::Null(), std::unordered_map<int, int>{});
	}

	static RCNAttribute Neutral(int asn)
	{
		return RCNAttribute(asn, RA::Neutral(), 0);
	}

	void addAsEntry(int asn, int seq)
	{
		base::addAsEntry(asn);
		ts[asn] = seq;
	}

	friend RCNAttribute operator+(const RCNAttribute &link, const RCNAttribute &a)
	{
		if (a.isInvalid())
		{
			return RCNAttribute::Invalid();
		}

		if (std::find(a.as_path.begin(), a.as_path.end(), link.as_path[0]) != a.as_path.end())
		{
			return RCNAttribute::Invalid();
		}

		typename base::Ext ext;

		std::vector<int> new_path(a.as_path);
		new_path.push_back(link.as_path[0]);

		auto new_ts             = a.ts;
		new_ts[link.as_path[0]] = link.ts.at(link.as_path[0]);

		return RCNAttribute(std::move(new_path), ext(link.attribute, a.attribute), std::move(new_ts));
	}

	friend std::ostream &operator<<(std::ostream &os, const std::vector<RCNAttribute> &v)
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
} // namespace Protocol::RCN

#endif
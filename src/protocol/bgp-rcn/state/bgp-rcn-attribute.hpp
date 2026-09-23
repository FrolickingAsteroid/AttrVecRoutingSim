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

	using Ext = RA::Ext;

	std::unordered_map<int, int> ts; // ts(u) for u in as_path

	RCNAttribute() = default;

	RCNAttribute(const int asn, base::A attr, const int seq)
	    : base(attr)
	{
		ts[asn] = seq;
	}

	RCNAttribute(base::A attr, std::unordered_map<int, int> ts_)
	    : base(attr)
	    , ts(std::move(ts_))
	{
	}

	static RCNAttribute Invalid()
	{
		return RCNAttribute(RA::Null(), std::unordered_map<int, int>{});
	}

	static RCNAttribute Neutral(int asn)
	{
		return RCNAttribute(asn, RA::Neutral(asn), 0);
	}

	friend RCNAttribute operator+(const RCNAttribute &link, const RCNAttribute &a)
	{
		base extended = static_cast<const base &>(link) + static_cast<const base &>(a);

		if (extended.isInvalid())
		{
			return RCNAttribute::Invalid();
		}

		// get new node
		const int new_node = link.getAttr().getPath()[0];

		auto new_ts      = a.ts;
		new_ts[new_node] = link.ts.at(new_node);

		return RCNAttribute(extended.getAttr(), std::move(new_ts));
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
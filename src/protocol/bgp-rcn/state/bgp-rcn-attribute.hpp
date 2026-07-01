#ifndef RCN_ATTRIBUTE_HPP
#define RCN_ATTRIBUTE_HPP

#include "protocol/bgp/state/bgp-attribute.hpp"

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <vector>
namespace Protocol::RCN
{
template <class A, class Ext, class Ord>
struct RCNAttribute : BGP::BGPAttribute<A, Ext, Ord> {
	using base = BGP::BGPAttribute<A, Ext, Ord>;

	std::unordered_map<int, int> ts; // ts(u) for u in as_path

	RCNAttribute() = default;

	RCNAttribute(int asn, A attr, const int seq)
		: base(asn, attr)
	{
		ts[asn] = seq;
	}

	RCNAttribute(std::vector<int> path, A attr, std::unordered_map<int, int> ts_)
		: base(std::move(path), attr)
		, ts(std::move(ts_))
	{
	}

	static RCNAttribute Invalid()
	{
		return RCNAttribute(
			std::vector<int>{}, Ord::Null(), std::unordered_map<int, int>{});
	}

	static RCNAttribute Neutral(int asn)
	{
		return RCNAttribute(asn, Ord::Neutral(), 0);
	}

	void addAsEntry(int asn, int seq)
	{
		base::addAsEntry(asn);
		ts[asn] = seq;
	}

	friend RCNAttribute operator+(const RCNAttribute &link, const RCNAttribute &a)
	{
		if (a.isInvalid())
			return RCNAttribute::Invalid();
		if (static_cast<int>(a.as_path.size()) > BGP::MaxASPathLength)
			return RCNAttribute::Invalid();
		if (std::find(a.as_path.begin(), a.as_path.end(), link.as_path[0]) !=
		    a.as_path.end())
			return RCNAttribute::Invalid();

		Ext		 ext;
		std::vector<int> new_path(a.as_path);
		new_path.push_back(link.as_path[0]);

		auto new_ts		= a.ts;
		new_ts[link.as_path[0]] = link.ts.at(link.as_path[0]);

		return RCNAttribute(std::move(new_path),
				    ext(link.attribute, a.attribute),
				    std::move(new_ts));
	}
};
}

#endif
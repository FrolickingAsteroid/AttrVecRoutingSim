#ifndef RCN_MESSAGE_HPP
#define RCN_MESSAGE_HPP

#include "protocol/bgp-rcn/state/bgp-rcn-attribute.hpp"

#include <optional>

namespace Protocol::RCN
{

struct rcn {
	int c;
	int ts;
};

template <class A, class Ext, class Ord> struct RCNMsg {
	using attr = RCNAttribute<A, Ext, Ord>;

	attr		   route;
	std::optional<rcn> RCN;

	RCNMsg() = default;

	explicit RCNMsg(attr route_)
		: route(std::move(route_))
	{
	}

	RCNMsg(attr route_, rcn rcn_)
		: route(std::move(route_))
		, RCN(rcn_)
	{
	}
};

}

#endif

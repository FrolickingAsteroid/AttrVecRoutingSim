#ifndef RCN_MESSAGE_HPP
#define RCN_MESSAGE_HPP

#include <optional>

#include "protocol/bgp-rcn/state/bgp-rcn-attribute.hpp"

namespace Protocol::RCN
{

struct rcn
{
	int c;
	int ts;
};

template <class RA>
struct RCNMsg
{
	using attr = RCNAttribute<RA>;

	attr               route;
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

} // namespace Protocol::RCN

#endif

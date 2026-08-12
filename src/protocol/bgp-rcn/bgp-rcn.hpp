#ifndef RCN_HPP
#define RCN_HPP

#include <functional>
#include <vector>

#include "protocol/bgp-rcn/state/bgp-rcn-attribute.hpp"
#include "protocol/bgp-rcn/state/node-rcn-state.hpp"
#include "protocol/bgp-rcn/state/rcn-msg.hpp"
#include "protocol/protocol.hpp"

namespace Protocol::RCN
{
template <class RA>
struct RCN : IProtocol<typename RA::Attr, RCNAttribute<RA>, RCNMsg<RA>>
{
	using A   = RA::BaseAttr;
	using Ext = RA::Ext;
	using Ord = RA::Ord;

	using attr = RCNAttribute<RA>;
	using Msg  = RCNMsg<RA>;

	using Base = IProtocol<A, attr, Msg>;

	using Base::cal;
	using Base::data;
	using Base::graph;

	using node_state = RCNNodeState<RA>;
	std::vector<node_state> s;

	explicit RCN(Graphs::Graph<A> &graph_)
	    : Base(graph_)
	    , s(graph_.V, node_state())
	{
	}

	void DestinationAnnouncement(int t);

	void LinkFailure(int u, int v) override;

	void NodeDown(int t);

	void MsgReception(int u, int v, Msg received) override;

	void process() override;
};
} // namespace Protocol::RCN

#include "protocol/bgp-rcn/bgp-rcn.tpp"

#endif
#ifndef RCN_HPP
#define RCN_HPP

#include "protocol/bgp-rcn/state/bgp-rcn-attribute.hpp"
#include "protocol/bgp-rcn/state/node-rcn-state.hpp"
#include "protocol/bgp-rcn/state/rcn-msg.hpp"
#include "protocol/protocol.hpp"

#include <functional>
#include <vector>

namespace Protocol::RCN
{

template <class RA>
struct RCN
	: IProtocol<typename RA::BaseAttr,
		    RCNAttribute<typename RA::BaseAttr, typename RA::Ext, typename RA::Ord>,
		    RCNMsg<typename RA::BaseAttr, typename RA::Ext, typename RA::Ord>> {
	using A	  = RA::BaseAttr;
	using Ext = RA::Ext;
	using Ord = RA::Ord;

	using attr = RCNAttribute<A, Ext, Ord>;
	using Msg  = RCNMsg<A, Ext, Ord>;

	using Base = IProtocol<A, attr, Msg>;

	using Base::graph;
	using Base::cal;
	using Base::data;
	using Base::Send;

	using node_state = RCNNodeState<A, Ext, Ord>;

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

	std::function<bool(int)> nodeConverged = nullptr;
	std::function<void()>	 converged     = nullptr;
};

}

#include "protocol/bgp-rcn/bgp-rcn.tpp"

#endif
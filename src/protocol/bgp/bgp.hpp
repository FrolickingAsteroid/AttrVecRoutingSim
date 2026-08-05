#ifndef BGP_HPP
#define BGP_HPP

#include <vector>

#include "protocol/bgp/state/bgp-attribute.hpp"
#include "protocol/bgp/state/node-state.hpp"
#include "protocol/protocol.hpp"

namespace Protocol::BGP
{
// BGP<A, Ord, Ext> implements the BGP path-vector protocol over IProtocol.
//
// Template parameters:
//   A    — the underlying metric type (e.g. int, Weight).
//          Must support operator+, operator<, operator!=, operator==.
//   Ord  — provides Ord::Neutral(int asn) → BGPAttribute<A> and
//                    Ord::Invalid()        → BGPAttribute<A>
//          Used only at DestinationAnnouncement and construction time.
//   Ext  — a callable: Ext()(link_attr, received_attr) → extended BGPAttribute<A>
//          Applied in MsgReception before handing the result to the node state.
//
// Cycle-detection bookkeeping (nCycles, inCycle, cycleFormed, timeInCycle)
// lives here, not in BGPNodeState, because it is protocol-level observation,
// not per-node routing state.

template <class RA>
struct BGP : IProtocol<typename RA::BaseAttr, BGPAttribute<typename RA::BaseAttr, typename RA::Ext, typename RA::Ord>>
{
	using Base =
	    IProtocol<typename RA::BaseAttr, BGPAttribute<typename RA::BaseAttr, typename RA::Ext, typename RA::Ord>>;
	using Base::cal;
	using Base::data;
	using Base::graph;

	using A   = RA::BaseAttr;
	using Ext = RA::Ext;
	using Ord = RA::Ord;

	using attr       = BGPAttribute<A, Ext, Ord>;
	using node_state = BGPNodeState<attr>;

	std::vector<node_state> s;

	explicit BGP(Graph<A> &graph_)
	    : Base(graph_)
	    , s(graph_.V, node_state())
	{
	}

	void DestinationAnnouncement(int t);

	void LinkFailure(int u, int v) override;

	void NodeDown(int t);

	void MsgReception(int u, int v, attr received) override;

	void process() override;

	std::function<bool(int)> nodeConverged = nullptr;
	std::function<void()>    converged     = nullptr;
};
} // namespace Protocol::BGP

#include "protocol/bgp/bgp.tpp"
#endif

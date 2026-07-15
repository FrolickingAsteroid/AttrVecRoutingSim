#ifndef BGP_HPP
#define BGP_HPP

#include "utils/ui/logger.hpp"

#include "protocol/bgp/state/bgp-attribute.hpp"
#include "protocol/bgp/state/node-state.hpp"
#include "protocol/protocol.hpp"
#include <vector>

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
struct BGP : IProtocol<typename RA::BaseAttr, BGPAttribute<typename RA::BaseAttr, typename RA::Ext, typename RA::Ord>> {
	using Base = IProtocol<typename RA::BaseAttr,
			       BGPAttribute<typename RA::BaseAttr, typename RA::Ext, typename RA::Ord>>;
	using Base::graph;
	using Base::cal;
	using Base::data;

	using A	  = RA::BaseAttr;
	using Ext = RA::Ext;
	using Ord = RA::Ord;

	using attr	 = BGPAttribute<A, Ext, Ord>;
	using node_state = BGPNodeState<attr>;

	std::vector<node_state> s;

	std::vector<int>  nCycles;
	std::vector<bool> inCycle;
	std::vector<int>  cycleFormed;
	std::vector<int>  timeInCycle;
	std::vector<bool> changeAttr;
	std::vector<bool> firstChange;

	explicit BGP(Graph<A> &graph_)
		: Base(graph_)
		, s(graph_.V, node_state())
		, nCycles(graph_.V, 0)
		, inCycle(graph_.V, false)
		, cycleFormed(graph_.V, 0)
		, timeInCycle(graph_.V, 0)
		, changeAttr(graph_.V, false)
		, firstChange(graph_.V, false)
	{
	}

	void DestinationAnnouncement(int t);

	void LinkFailure(int u, int v) override;
	void NodeDown(int t);
	void MsgReception(int u, int v, attr received) override;

	void process() override;

	std::function<bool(int)> nodeConverged = nullptr;
	std::function<void()>	 converged     = nullptr;

	//std::vector<int> &CycleBlackHoleTimes();
	//std::vector<int> &CycleBlackHoleChanges();

	// private:
	// Fan out the elected attribute of node u to all its in-neighbors.
	// void advertiseToNeighbors(int u, int dest);

	// Resets all cycle-detection bookkeeping to zero.
	// Called at the start of DestinationAnnouncement and Withdrawal.
	// void resetCycleState();

	// Updates inCycle/nCycles/timeInCycle for all nodes at time t.
	// Mirrors the check block at the end of processEvent in the original.
	// void checkCycles(int t);

	// Recursive cycle check for node u. Mirrors auxCheckCycle.
	// bool auxCheckCycle(int u, std::vector<bool> &vis, std::vector<bool> &inStack);
};

}

#include "protocol/bgp/bgp.tpp"
#endif
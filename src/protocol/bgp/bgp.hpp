#ifndef BGP_HPP
#define BGP_HPP

#include <vector>

#include "protocol/bgp/state/bgp-attribute.hpp"
#include "protocol/bgp/state/node-state.hpp"
#include "protocol/protocol.hpp"

namespace Protocol::BGP
{
template <class RA>
struct BGP : IProtocol<typename RA::Attr, BGPAttribute<RA>>
{

	using BGPAttr= BGPAttribute<RA>;

	using Base = IProtocol<typename RA::Attr, BGPAttr>;

	using Base::cal;
	using Base::data;
	using Base::graph;

	using node_state = BGPNodeState<BGPAttr>;

	std::vector<node_state> s;

	explicit BGP(Graphs::Graph<typename RA::Attr> &graph_)
	    : Base(graph_)
	    , s(graph_.V, node_state())
	{
	}

	void DestinationAnnouncement(int t);

	void LinkFailure(int u, int v) override;

	void NodeDown(int t);

	void MsgReception(int u, int v, BGPAttr received) override;

	void process() override;
};
} // namespace Protocol::BGP

#include "protocol/bgp/bgp.tpp"
#endif

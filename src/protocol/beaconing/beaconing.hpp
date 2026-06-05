#ifndef BEACONING_HPP
#define BEACONING_HPP

#include "protocol/beaconing/state/beacon.hpp"
#include "protocol/beaconing/state/store.hpp"
#include "protocol/protocol.hpp"


namespace Protocol::Beaconing
{
template <class RA> struct BECING : IProtocol<typename RA::BaseAttr, Beacon<typename RA::BaseAttr>> {
	using Base = IProtocol<typename RA::BaseAttr, Beacon<typename RA::BaseAttr>>;
	using Base::graph;
	using Base::cal;
	using Base::data;

	using A = RA::BaseAttr;
	using Ord = RA::Ord;
	using Ext = RA::Ext;

	std::vector<BeaconStore<A, Ord>> s;

	explicit BECING(Graphs::Graph<A> &graph_)
		: Base(graph_)
		, s(graph_.V, BeaconStore<A, Ord>())
	{
	}

	void DestinationAnnouncement(int t, int n, double period, int max) override;

	void MsgReception(int u, int v, Beacon<A> beacon) override;
	void SchedulePropagation(int u, double d_prop, int max);
	void Propagate(int u);
};
}

#include "protocol/beaconing/beaconing.tpp"

#endif

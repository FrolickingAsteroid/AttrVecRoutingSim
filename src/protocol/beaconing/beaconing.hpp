#ifndef BEACONING_HPP
#define BEACONING_HPP

#include "protocol/beaconing/state/beacon.hpp"
#include "protocol/beaconing/state/store.hpp"
#include "protocol/protocol.hpp"
#include <vector>

namespace Protocol::Beaconing
{
template <class A, class Ord, class Ext>
struct BECING : IProtocol<A, Beacon<A> > {
	// https://github.com/scionproto/scion/blob/control/config/config.go#L37
	static constexpr double DPropInterval = 5000; // 5 seconds each
	static constexpr double DefaultRegistrationInterval = 5000;

	using Base = IProtocol<A, Beacon<A> >;
	using Base::graph;
	using Base::cal;
	using Base::data;

	using b = Beacon<A>;
	using store = BeaconStore<A, Ord>;
	using seq_attr = A;

	std::vector<store> s;

	explicit BECING(Graphs::Graph<A> &graph_)
		: Base(graph_)
		, s(graph_.V, store())
	{
	}

	void DestinationAnnouncement(int t, int n, double period,
				     int max) override;
	void LinkFailure(int u, int v) override;
	void MsgReception(int u, int v, b beacon);
	void SchedulePropagation(int u, int max);
	void Propagate(int u);
};
} // namespace Protocol::Beaconing

#include "protocol/beaconing/beaconing.tpp"

#endif

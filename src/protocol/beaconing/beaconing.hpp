#ifndef BEACONING_HPP
#define BEACONING_HPP

#include "protocol/beaconing/state/beacon.hpp"
#include "protocol/beaconing/state/store.hpp"
#include "protocol/protocol.hpp"

namespace Protocol::Beaconing
{
template <class RA>
struct BEACONING : IProtocol<typename RA::Attr, Beacon<RA>>
{
	using Base = IProtocol<typename RA::Attr, Beacon<RA>>;

	using Base::cal;
	using Base::data;
	using Base::graph;

	using Bcn = Beacon<RA>;

	std::vector<BeaconStore<RA>> s;

	explicit BEACONING(Graphs::Graph<typename RA::Attr> &graph_)
	    : Base(graph_)
	    , s(graph_.V, BeaconStore<RA>())
	{
	}

	void DestinationAnnouncement(int t, int n, double period, std::optional<int> max);
	void SchedulePropagation(int u, double d_prop, double horizon);
	void Propagate(int u);

	void MsgReception(int u, int v, Bcn beacon) override;
	void LinkFailure(int u, int v) override;

	void process() override;

	// Callbacks
	std::function<void(int u, double time, const std::vector<Beacon<RA>> &best,
			   const std::vector<Beacon<RA>> &candidates, std::string tag)>
	    record = nullptr;
};
} // namespace Protocol::Beaconing

#include "protocol/beaconing/beaconing.tpp"

#endif

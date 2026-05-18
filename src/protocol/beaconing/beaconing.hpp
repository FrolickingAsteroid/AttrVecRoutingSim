#ifndef BEACONING_HPP
#define BEACONING_HPP

#include "protocol/beaconing/state/beacon.hpp"
#include "protocol/beaconing/state/store.hpp"
#include "protocol/protocol.hpp"
#include <vector>

namespace Protocol::Beaconing
{
template <class A, class Ord, class Ext> struct BECING : IProtocol<A> {
    // https://github.com/scionproto/scion/blob/control/config/config.go#L37
    static constexpr double DPropInterval = 5000; // 5 seconds each
    static constexpr double DefaultRegistrationInterval = 5000;

    using IProtocol<A>::graph;
    using IProtocol<A>::cal;
    using IProtocol<A>::data;

    using b = Beacon<A>;
    using store = BeaconStore<A, Ord>;
    using seq_attr = RoutingAlgebras::SeqAttr<A>;

    std::vector<store> s;

    explicit BECING(Graphs::Graph<A> &graph_)
	    : IProtocol<A>(graph_)
	    , s(graph_.V, store())
    {
    }

    void DestinationAnnouncement(int t, int n, double period, int max) override;
    void MsgReception(int u, int v, b beacon);
    void SchedulePropagation(int u);
    void Propagate(int u);
};
} // namespace Protocol::Beaconing

#include "protocol/rpbob/rpbob.tpp"

#endif

#ifndef BEACONING_TPP
#define BEACONING_TPP

#include <iostream>

#include "protocol/beaconing/beaconing.hpp"

namespace Protocol::Beaconing {
    template<class RA>
    void BECING<RA>::DestinationAnnouncement(int t, int n, double period, int max) {
        Beacon<A> beacon(t, Ord::Neutral());
        beacon.setTimeout(cal.now());

        this->Send(t, beacon);

        // schedule next routing computation
        n += 1;
        double d = period * n;
        if (n < max) {
            cal.schedule(d, [t, n, period, max, this]() mutable {
                this->DestinationAnnouncement(t, n, period, max);
            });
        }
    }

    template<class RA>
    void BECING<RA>::MsgReception(int u, int v, Beacon<A> beacon) {
        if (beacon.expired(cal.now())) {
            return;
        }
        Ext extension;

        A lw(std::get < 2 > (graph.adjListOut[u].at(v)));
        A candidate = extension(lw, beacon.getAttr());
        Beacon<A> old_best = s[u].best_candidate;

        beacon.addAsEntry(u);
        beacon.addAttr(candidate);
        s[u].Insert(beacon);

        if (old_best != s[u].best_candidate) {
            data.recordChange(u, cal.now(), s[u].best_candidate);
        }
    }

    template<class RA>
    void BECING<RA>::SchedulePropagation(int u, double d_prop, int max) {
        cal.schedule(cal.now(), [this, u]() {
            this->Propagate(u);
        });

        if (cal.now() + d_prop < 1000)
            cal.schedule(cal.now() + d_prop, [this, u, d_prop, max]() {
                this->SchedulePropagation(u, d_prop, max);
            });
    }

    template<class RA>
    void BECING<RA>::Propagate(int u) {
        auto selected = s[u].BeaconsToPropagate(cal.now());

        if (selected.empty())
            return;

        for (auto beacon: selected)
            this->Send(u, beacon);
    }
} // namespace Protocol::Beaconing
#endif
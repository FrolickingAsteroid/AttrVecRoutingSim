#ifndef BEACONING_TPP
#define BEACONING_TPP

#include "protocol/beaconing/beaconing.hpp"

namespace Protocol::Beaconing
{
template <class A, class Ord, class Ext>
void BECING<A, Ord, Ext>::LinkFailure(int u, int v)
{
	graph.deactivate(u, v);
}
template <class A, class Ord, class Ext>
void BECING<A, Ord, Ext>::DestinationAnnouncement(int t, int n, double period,
						  int max)
{
	b beacon(t, Ord::Neutral());
	beacon.setTimeout(cal.now());

	// bootstrap periodic propagation on every node
	if (n == 0) {
		for (int u = 0; u < graph.V; ++u)
			SchedulePropagation(u, max);
	}

	for (auto &[r, edge] : graph.adjListIn[t]) {
		if (auto &[delay, active, attr] = edge; active) {
			cal.schedule(cal.now() + delay,
				     [r, t, beacon, this]() mutable {
					     this->MsgReception(r, t, beacon);
				     });
		}
	}

	// schedule next routing computation
	n += 1;
	double d = period * n;
	if (n < max) {
		cal.schedule(d, [t, n, period, max, this]() mutable {
			this->DestinationAnnouncement(t, n, period, max);
		});
	}
}

template <class A, class Ord, class Ext>
void BECING<A, Ord, Ext>::MsgReception(int u, int v, b beacon)
{
	if (beacon.expired(cal.now())) {
		return;
	}
	Ext extension;

	seq_attr lw(std::get<2>(graph.adjListOut[u].at(v)));
	seq_attr candidate = extension(lw, beacon.getAttr());

	beacon.addAsEntry(u);
	beacon.addAttr(candidate);
	s[u].Insert(beacon);
}

template <class A, class Ord, class Ext>
void BECING<A, Ord, Ext>::SchedulePropagation(int u, int max)
{
	cal.schedule(cal.now(), [this, u]() { this->Propagate(u); });

	if (cal.now() < max)
		cal.schedule(cal.now() + DPropInterval, [this, u, max]() {
			this->SchedulePropagation(u, max);
		});
}

template <class A, class Ord, class Ext>
void BECING<A, Ord, Ext>::Propagate(int u)
{
	auto selected = s[u].BeaconsToPropagate(cal.now());
	if (selected.empty())
		return;

	data.record(cal.now(), u, selected.at(0), selected);

	for (auto &[v, edge] : this->graph.adjListOut[u]) {
		if (auto &[delay, active, attr] = edge; active) {
			for (auto beacon : selected) {
				cal.schedule(cal.now() + delay,
					     [this, u, v, beacon]() mutable {
						     this->MsgReception(u, v,
									beacon);
					     });
			}
		}
	}
}

} // namespace Protocol::Beaconing
#endif
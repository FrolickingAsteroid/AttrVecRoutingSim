#ifndef BEACONING_TPP
#define BEACONING_TPP

#include "protocol/beaconing/beaconing.hpp"

#include <iostream>

namespace Protocol::Beaconing
{
template <class A, class Ord, class Ext>
void BECING<A, Ord, Ext>::DestinationAnnouncement(int t, int n, double period,
						  int max)
{
	b beacon(t, Ord::Neutral());
	beacon.setTimeout(cal.now());

	uint64_t cnt = this->Send(t, beacon);
	data.record(cal.now(), t, "announcement", cnt);

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
	b old_best = s[u].best_candidate;

	beacon.addAsEntry(u);
	beacon.addAttr(candidate);
	s[u].Insert(beacon);

	if (u == 12) {
		b best = s[u].best_candidate;
	}



	if (old_best != s[u].best_candidate) {
		data.record(cal.now(), u, s[u].best_candidate);
	}
}

template <class A, class Ord, class Ext>
void BECING<A, Ord, Ext>::SchedulePropagation(int u, double d_prop, int max)
{
	cal.schedule(cal.now(), [this, u]() { this->Propagate(u); });

	if (cal.now() + d_prop < 500)
		cal.schedule(cal.now() + d_prop, [this, u, d_prop, max]() {
			this->SchedulePropagation(u, d_prop, max);
		});
}

template <class A, class Ord, class Ext>
void BECING<A, Ord, Ext>::Propagate(int u)
{
	auto selected = s[u].BeaconsToPropagate(cal.now());

	if (selected.empty())
		return;

	data.record(cal.now(), u, selected.at(0), selected);

	uint64_t cnt = 0;
	for (auto beacon : selected)
		cnt += this->Send(u, beacon);

	data.record(cal.now(), u, "propagate", cnt);
}

}
#endif
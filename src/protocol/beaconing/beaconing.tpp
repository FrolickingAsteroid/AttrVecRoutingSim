#ifndef BEACONING_TPP
#define BEACONING_TPP

#include <iostream>

#include "protocol/beaconing/beaconing.hpp"
#include "utils/helpers/helpers.tpp"

namespace Protocol::Beaconing
{

template <class RA>
void BEACONING<RA>::process()
{
	for (int u = 0; u < graph.V; ++u)
	{
		auto it  = data.finalValue.find(u);
		auto opt = s[u].GetOpt();

		if (opt.empty())
		{
			continue;
		}

		if (it == data.finalValue.end() || !utils::helpers::CompareOpt<Bcn>(opt, it->second))
		{
			data.recordChange(u, cal.now(), opt);
		}
	}
}

template <class RA>
void BEACONING<RA>::DestinationAnnouncement(int t, int n, double period, std::optional<int> max)
{

	s[t].SetOrigin(t, cal.now());

	// destination has a single opt attr: (e, t)
	auto o = s[t].GetOpt().front();
	this->Send(t, o);

	// schedule next routing computation
	n += 1;
	double d = period * n;

	const bool should_continue = max.has_value() ? (n < *max) : true;

	if (should_continue)
	{
		cal.schedule(d, [t, n, period, max, this]() mutable {
			this->DestinationAnnouncement(t, n, period, max);
		});
	}
}

template <class RA>
void BEACONING<RA>::LinkFailure(int u, int v)
{
	graph.deactivate(u, v);
}

template <class RA>
void BEACONING<RA>::MsgReception(int u, int v, Bcn beacon)
{
	// reject expired beacons
	if (beacon.expired(cal.now()))
	{
		return;
	}

	typename RA::Attr edgeAttr(graph.edgeWeight(u, v), u); // (l_attr, u)
	auto              extended = beacon + edgeAttr;        // (l_attr (ext) r_attr, uP)

	s[u].Insert(beacon);
	converged();
}

template <class RA>
void BEACONING<RA>::SchedulePropagation(int u, double d_prop, int max)
{
	cal.schedule(cal.now(), [this, u]() {
		this->Propagate(u);
	});

	// will run while not converged, must find something more suitable
	cal.schedule(cal.now() + d_prop, [this, u, d_prop, max]() {
		this->SchedulePropagation(u, d_prop, max);
	});
}

template <class RA>
void BEACONING<RA>::Propagate(int u)
{
	auto selected = s[u].BeaconsToPropagate(cal.now());

	if (selected.empty())
		return;

	for (auto beacon : selected)
		this->Send(u, beacon);
}
} // namespace Protocol::Beaconing
#endif
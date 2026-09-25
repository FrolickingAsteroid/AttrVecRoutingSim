#ifndef STORE_TPP
#define STORE_TPP

#include <algorithm>

#include "protocol/beaconing/state/store.hpp"
#include "utils/helpers/helpers.tpp"

namespace Protocol::Beaconing
{

template <class RA>
std::vector<typename BeaconStore<RA>::beacon> BeaconStore<RA>::GetBest() const
{
	return best_candidates;
}

template <class RA>
std::vector<typename BeaconStore<RA>::beacon> BeaconStore<RA>::GetCandidates() const
{
	return candidates;
}

template <class RA>
void BeaconStore<RA>::SetOrigin(int t, double time)
{
	A e = RA::Neutral(t);

	best_candidates = {beacon(e, time)};
	candidates      = best_candidates;
}
template <class RA>
void BeaconStore<RA>::Insert(beacon b)
{
	if (b.isInvalid())
	{
		return;
	}

	Ord ord;

	// check whether the beacon as already been registered in the store
	auto exists = std::find_if(candidates.begin(), candidates.end(), [&b, &ord](const beacon &current) {
		return (ord(current.getAttr(), b.getAttr()) == RoutingAlgebras::Comp::Equal);
	});

	// if this is the case and the new beacon has a larger TTL, update it
	if (exists != candidates.end())
	{
		if (b.timeout > exists->timeout)
		{
			exists->timeout = b.timeout;
		}
	}
	else
	{
		candidates.emplace(candidates.begin(), b);
	}

	std::sort(candidates.begin(), candidates.end(), [ord, b](const beacon &l, const beacon &r) {
		RoutingAlgebras::Comp c = ord(l.getAttr(), r.getAttr());
		if (c != RoutingAlgebras::Comp::Equal && c != RoutingAlgebras::Comp::Incomparable)
		{
			return c == RoutingAlgebras::Comp::Better; // strictly better attributes sorts first
		}
		if (l.getTimeout() != r.getTimeout())
		{
			return l.getTimeout() > r.getTimeout(); // larger timeout sorts first
		}
		return &l == &b; //  beacons that have just been recied sort first
	});

	best_candidates = SelectBeacons();
}

template <class RA>
bool BeaconStore<RA>::IsEmpty()
{
	return candidates.empty();
}

template <class RA>
void BeaconStore<RA>::Remove(double now)
{
	if (IsEmpty())
	{
		return;
	}

	candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
					[now](beacon &b) {
		return b.expired(now);
	}),
			 candidates.end());
}

template <class RA>
std::vector<Beacon<RA>> BeaconStore<RA>::BeaconsToPropagate(double now)
{
	// remove beacons that have expired
	Remove(now);

	if (IsEmpty())
	{
		best_candidates = {};
	}
	else
	{
		best_candidates = SelectBeacons();
	}

	return best_candidates;
}

template <class RA>
std::vector<Beacon<RA>> BeaconStore<RA>::SelectBeacons()
{
	if (static_cast<int>(candidates.size()) <= BestSetSize)
	{
		return candidates;
	}

	// take the "BestSetSize" shortest (sorted by length, in principle)
	std::vector<beacon> result(candidates.begin(), candidates.begin() + BestSetSize);

	return result;
}
} // namespace Protocol::Beaconing

#endif
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
void BeaconStore<RA>::SetOrigin(int t, double time)
{
	A e = RA::Neutral(t);

	best_candidates = {beacon(e, time)};
	candidates      = best_candidates;
}
template <class RA>
void BeaconStore<RA>::Insert(Beacon<RA> b)
{
	if (b.isInvalid())
	{
		return;
	}

	Ord ord;

	// check whether the beacon as already been registered in the store
	auto exists = std::find_if(candidates.begin(), candidates.end(), [&b, &ord](const Beacon<A> &current) {
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
		// If the beacon is new, insert it into the store (while preserving order)
		auto pos = candidates.begin();
		while (pos != candidates.end() && !RoutingAlgebras::BetterOrEqual(b.getAttr(), pos->getAttr()))
		{
			++pos;
		}

		candidates.insert(pos, b);
	}

	auto new_best = SelectBeacons();
	if (utils::helpers::CompareOpt<beacon>(new_best, best_candidates))
	{
		best_candidates = new_best;
	}
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
		return {};
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
	std::vector<Beacon<A>> result(candidates.begin(), candidates.begin() + BestSetSize);

	return result;
}
} // namespace Protocol::Beaconing

#endif
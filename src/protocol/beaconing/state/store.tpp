#ifndef STORE_TPP
#define STORE_TPP

#include "protocol/beaconing/state/store.hpp"
#include <algorithm>

namespace Protocol::Beaconing
{
template <class A, class Ord> void BeaconStore<A, Ord>::Insert(Beacon<A> bea)
{
	Ord ord;

	auto it = std::find_if(candidates.begin(), candidates.end(),
			       [&bea](const Beacon<A> &e) {
				       return e.path == bea.path;
			       });

	if (it != candidates.end()) {
		if (!ord(bea.attribute, it->attribute)) {
			it->timeout = bea.timeout;
			return;
		}
		candidates.erase(it);
	}

	auto pos = std::lower_bound(
		candidates.begin(), candidates.end(), bea,
		[&ord](const Beacon<A> &e, const Beacon<A> &val) {
			return ord(e.attribute, val.attribute);
		});

	candidates.insert(pos, bea);

	if (candidates.front() != best_candidate)
		best_candidate = candidates.front();
}

template <class A, class Ord> bool BeaconStore<A, Ord>::IsEmpty()
{
	return candidates.empty();
}

template <class A, class Ord> void BeaconStore<A, Ord>::Remove(double now)
{
	if (IsEmpty()) {
		return;
	}

	candidates.erase(
		std::remove_if(candidates.begin(), candidates.end(),
			       [now](beacon &b) { return b.expired(now); }),
		candidates.end());
}

template <class A, class Ord>
std::vector<Beacon<A> > BeaconStore<A, Ord>::BeaconsToPropagate(double now)
{
	Remove(now);
	if (IsEmpty())
		return {};
	return SelectBeacons();
}

template <class A, class Ord>
std::vector<Beacon<A> > BeaconStore<A, Ord>::SelectBeacons()
{
	if ((int)candidates.size() <= BestSetSize) {
		return candidates;
	}
	// take the "BestSetSize - 1" shortest (sorted by length)
	std::vector<Beacon<A> > result(candidates.begin(),
				       candidates.begin() + BestSetSize - 1);

	return result;
}

}

#endif
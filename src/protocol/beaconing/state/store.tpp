#ifndef STORE_TPP
#define STORE_TPP

#include "protocol/beaconing/state/store.hpp"
#include <algorithm>

namespace Protocol::Beaconing
{
template <class A, class Ord> void BeaconStore<A, Ord>::Insert(Beacon<A> b)
{
	for (auto &existing : candidates) {
		if (existing.path == b.path) {
			existing = b;
			return;
		}
	}
	candidates.push_back(b);
}

template <class A, class Ord>
std::vector<Beacon<A> > BeaconStore<A, Ord>::BeaconsToPropagate(double now)
{
	Ord ord;
	std::vector<Beacon<A> > sorted = candidates;
	std::sort(sorted.begin(), sorted.end(),
		  [ord](const Beacon<A> &a, const Beacon<A> &b) {
			  return ord(a.attribute, b.attribute);
		  });

	if (static_cast<int>(sorted.size()) > CandidateSetSize)
		sorted.resize(CandidateSetSize);

	// filter expired
	std::vector<Beacon<A> > fresh;
	fresh.reserve(sorted.size());
	for (auto &b : sorted) {
		if (!b.expired(now)) {
			fresh.push_back(b);
		}
	}
	return SelectBeacons(fresh);
}

template <class A, class Ord>
std::vector<Beacon<A> > BeaconStore<A, Ord>::BeaconsToRegister(double now) const
{
	return BeaconsToPropagate(now);
}

template <class A, class Ord>
std::vector<Beacon<A> >
BeaconStore<A, Ord>::SelectBeacons(const std::vector<Beacon<A> > &beacons)
{
	if ((int)beacons.size() <= BestSetSize)
		return beacons;

	// take the "BestSetSize - 1" shortest (sorted by length)
	std::vector<Beacon<A> > result(beacons.begin(),
				       beacons.begin() + BestSetSize - 1);

	return result;
}

} // namespace Protocol::Beaconing

#endif
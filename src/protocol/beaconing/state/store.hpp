#ifndef STORE_HPP
#define STORE_HPP

#include "beacon.hpp"
#include <vector>

namespace Protocol::Beaconing
{
template <class A, class Ord> struct BeaconStore {
	static constexpr int CandidateSetSize = 100;
	static constexpr int BestSetSize = 20;

	using beacon = Beacon<A>;

	std::vector<beacon> candidates;
	beacon best_candidate;

	BeaconStore()
		: best_candidate(beacon(-1, Ord::Null()))
	{
	}

	// Insertion seems to be unbounded
	void Insert(beacon bea);

	// https://github.com/scionproto/scion/blob/master/control/beaconing/propagator.go#L110
	bool IsEmpty();

	void Remove(double now);

	// https://github.com/scionproto/scion/blob/master/control/beacon/store.go#L156
	std::vector<beacon> BeaconsToPropagate(double now);

	// https://github.com/scionproto/scion/blob/master/control/beacon/store.go#L131

	// https://github.com/scionproto/scion/blob/master/control/beacon/selection_algo.go
	std::vector<beacon> SelectBeacons();
};
} // namespace Protocol::Beaconing

#include "store.tpp"
#endif
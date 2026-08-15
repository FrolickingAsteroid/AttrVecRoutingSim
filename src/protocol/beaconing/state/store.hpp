#ifndef STORE_HPP
#define STORE_HPP

#include <vector>

#include "beacon.hpp"

namespace Protocol::Beaconing
{
template <class RA>
class BeaconStore
{
	static constexpr int CandidateSetSize = 100; // degault is 100
	static constexpr int BestSetSize      = 1;   // default is 20

	using beacon = Beacon<RA>;
	using A      = RA::Attr;
	using Ord    = RA::Ord;

    public:
	BeaconStore()
	{
	}

	void SetOrigin(int t, double timeout_);

	std::vector<beacon> GetBest() const;

	// Insertion is unbounded. Every beacon is inserted into the beacon store, provided that the path they transport
	// is a simple path
	void Insert(beacon b);

	// https://github.com/scionproto/scion/blob/master/control/beaconing/propagator.go#L110
	bool IsEmpty();

	void Remove(double now);

	// https://github.com/scionproto/scion/blob/master/control/beacon/store.go#L156
	std::vector<beacon> BeaconsToPropagate(double now);

	// https://github.com/scionproto/scion/blob/master/control/beacon/store.go#L131

	// https://github.com/scionproto/scion/blob/master/control/beacon/selection_algo.go
	std::vector<beacon> SelectBeacons();

    private:
	std::vector<beacon> candidates;
	std::vector<beacon> best_candidates;
};
} // namespace Protocol::Beaconing

#include "store.tpp"
#endif
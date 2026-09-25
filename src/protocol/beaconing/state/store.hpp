#ifndef STORE_HPP
#define STORE_HPP

#include <vector>

#include "beacon.hpp"

namespace Protocol::Beaconing
{
template <class RA>
class BeaconStore
{
	static constexpr int CandidateSetSize = 100; // default is 100
	static constexpr int BestSetSize      = 1;   // default is 20

	using beacon = Beacon<RA>;
	using A      = RA::Attr;
	using Ord    = RA::Ord;

    public:
	BeaconStore() = default;

	void SetOrigin(int t, double timeout_);

	std::vector<beacon> GetBest() const;
	std::vector<beacon> GetCandidates() const;

	// Insertion is unbounded. Every beacon is inserted into the beacon store,
	// provided that the path they transport is a simple path
	void Insert(beacon b);

	// https://github.com/scionproto/scion/blob/master/control/beaconing/propagator.go#L110
	bool IsEmpty();

	void Remove(double now);

	// https://github.com/scionproto/scion/blob/master/control/beacon/store.go#L156
	std::vector<beacon> BeaconsToPropagate(double now);

	// https://github.com/scionproto/scion/blob/master/control/beacon/store.go#L131

	// https://github.com/scionproto/scion/blob/master/control/beacon/selection_algo.go
	std::vector<beacon> SelectBeacons();

	friend std::ostream &operator<<(std::ostream &os, const BeaconStore &sw)
	{
		const auto &best = sw.GetBest();
		os << "best: [";
		for (std::size_t i = 0; i < best.size(); ++i)
		{
			os << best[i];
			if (i + 1 < best.size())
			{
				os << ", ";
			}
		}
		os << "]";

		os << " candidates: [";
		const auto &set = sw.GetCandidates();
		for (std::size_t i = 0; i < set.size(); ++i)
		{
			os << set[i];
			if (i + 1 < set.size())
			{
				os << ",";
			}
		}
		os << "]";
		return os;
	}

    private:
	std::vector<beacon> candidates;
	std::vector<beacon> best_candidates;
};
} // namespace Protocol::Beaconing

#include "store.tpp"
#endif
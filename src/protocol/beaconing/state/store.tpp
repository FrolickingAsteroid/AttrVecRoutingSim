#ifndef STORE_TPP
#define STORE_TPP

#include "protocol/beaconing/state/store.hpp"
#include <algorithm>

namespace Protocol::Beaconing
{
template <class A, class Ord> void BeaconStore<A, Ord>::Insert(Beacon<A> b)
{
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

    // take the "BestSetSize - 1" shortest (sorted by hop count)
    std::vector<Beacon<A> > result(beacons.begin(),
				   beacons.begin() + BestSetSize - 1);

    /* diversity of result set measured against best smallest length
        int currentDiversity = Diversity(result[0], result[0]);
        for (int i = 1; i < (int) result.size(); ++i)
            currentDiversity = std::max(currentDiversity, Diversity(result[0], result[i]));

        // find most diverse beacon in the remainder
        const Beacon<A> *mostDiverse = nullptr;
        int maxDiversity = -1;
        int minLen = std::numeric_limits<int>::max();

        for (int i = BestSetSize - 1; i < (int) beacons.size(); ++i) {
            int d = Diversity(result[0], beacons[i]);
            int l = (int) beacons[i].path.size();
            if (d > maxDiversity || (d == maxDiversity && l < minLen)) {
                maxDiversity = d;
                minLen = l;
                mostDiverse = &beacons[i];
            }
        }

        // only use diverse beacon if it improves over what's already in result
        if (mostDiverse != nullptr && maxDiversity > currentDiversity)
            result.push_back(*mostDiverse);
        else
            result.push_back(beacons[BestSetSize - 1]);
        */
    return result;
}

} // namespace Protocol::Beaconing

#endif
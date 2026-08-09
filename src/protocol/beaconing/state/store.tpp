#ifndef STORE_TPP
#define STORE_TPP

#include <algorithm>

#include "protocol/beaconing/state/store.hpp"

namespace Protocol::Beaconing {
    template<class A, class Ord>
    void BeaconStore<A, Ord>::Insert(Beacon<A> bea) {
        Ord ord;

        auto better = [&ord](const A &a, const A &b) {
            return ord(a, b) && !ord(b, a);
        };

        auto same_path = std::find_if(candidates.begin(), candidates.end(), [&bea](const Beacon<A> &e) {
            return e.path == bea.path;
        });

        if (same_path != candidates.end()) {
            if (bea.timeout > same_path->timeout)
                same_path->timeout = bea.timeout;

            return;
        }

        auto pos = candidates.begin();

        while (pos != candidates.end() && !better(bea.attribute, pos->attribute)) {
            ++pos;
        }

        candidates.insert(pos, bea);
        best_candidate = candidates.front();
    }

    template<class A, class Ord>
    bool BeaconStore<A, Ord>::IsEmpty() {
        return candidates.empty();
    }

    template<class A, class Ord>
    void BeaconStore<A, Ord>::Remove(double now) {
        if (IsEmpty()) {
            return;
        }

        candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
                                        [now](beacon &b) {
                                            return b.expired(now);
                                        }),
                         candidates.end());
    }

    template<class A, class Ord>
    std::vector<Beacon<A> > BeaconStore<A, Ord>::BeaconsToPropagate(double now) {
        Remove(now);
        if (IsEmpty())
            return {};
        return SelectBeacons();
    }

    template<class A, class Ord>
    std::vector<Beacon<A> > BeaconStore<A, Ord>::SelectBeacons() {
        if ((int) candidates.size() <= BestSetSize) {
            return candidates;
        }
        // take the "BestSetSize - 1" shortest (sorted by length)
        std::vector<Beacon<A> > result(candidates.begin(), candidates.begin() + BestSetSize);

        return result;
    }
} // namespace Protocol::Beaconing

#endif
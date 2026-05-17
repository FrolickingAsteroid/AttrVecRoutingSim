#ifndef STORE_HPP
#define STORE_HPP

#include "beacon.hpp"
#include <vector>

namespace Protocol::Beaconing {
    template<class A, class Ord>
    struct BeaconStore {

        static constexpr int CandidateSetSize = 100;
        static constexpr int BestSetSize = 20;

        std::vector<Beacon<A>> candidates;

        // Insertion seems to be unbounded
        void Insert(Beacon<A> b);

        // https://github.com/scionproto/scion/blob/master/control/beacon/store.go#L156
        std::vector<Beacon<A>> BeaconsToPropagate(double now);


        // https://github.com/scionproto/scion/blob/master/control/beacon/store.go#L131
        std::vector<Beacon<A>> BeaconsToRegister(double now) const;

        // https://github.com/scionproto/scion/blob/master/control/beacon/selection_algo.go
        std::vector<Beacon<A>> SelectBeacons(const std::vector<Beacon<A>> &beacons);
    };
}// namespace Protocol::Beaconing

#include "store.tpp"
#endif
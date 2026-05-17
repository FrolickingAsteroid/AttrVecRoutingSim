#ifndef BEACON_HPP
#define BEACON_HPP

#include <routing-algebras/sequenced-ra.hpp>
#include <vector>


namespace Protocol::Beaconing {
    template<class A>
    struct Beacon {
        RoutingAlgebras::SeqAttr<A> attribute;
        std::vector<int> path;

        static constexpr int expiration = 10;//(no idea)

        double timeout;

        Beacon(int e, RoutingAlgebras::SeqAttr<A> a) : attribute(a) {
            addAsEntry(e);
        }

        bool expired(double now) {
            return now > timeout;
        }

        void setTimeout(double time_) {
            timeout = time + expiration;
        }

        void addAsEntry(const int &entry) {
            path.insert(path.begin(), entry);
        }


        RoutingAlgebras::SeqAttr<A> getAttr() {
            return attribute;
        }

        void addAttr(const RoutingAlgebras::SeqAttr<A> &attr) {
            attribute = attr;
        }

        bool operator==(const Beacon<A> &other) const {
            return attribute == other.attribute &&
                   path == other.path;
        }

        bool operator!=(const Beacon<A> &other) const {
            return !(*this == other);
        }
    };
}// namespace Protocol::Beaconing

#endif

#ifndef MP_PATH_SEGMENT_H
#define MP_PATH_SEGMENT_H

#include <routing-algebras/sequenced-ra.hpp>
#include <ns3/vector.h>
#include <vector>


namespace Protocol::State {
    template<class A>
    struct Path {
        RoutingAlgebras::SeqAttr<A> attribute;
        std::vector<int> path;

        Path(int e, RoutingAlgebras::SeqAttr<A> a) : attribute(a) {
            addAsEntry(e);
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

        bool operator==(const Path<A> &other) const {
            return attribute == other.attribute &&
                   path == other.path;
        }

        bool operator!=(const Path<A> &other) const {
            return !(*this == other);
        }
    };
} // namespace ProtocolState

#endif

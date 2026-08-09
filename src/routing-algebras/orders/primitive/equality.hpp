#ifndef EQUALITY_HPP
#define EQUALITY_HPP

#include "routing-algebras/attributes/paths.hpp"

namespace RoutingAlgebras {
    template<class Node>
    struct PathEquality {
        bool operator()(const Path<Node> &p, const Path<Node> &q) const {
            return p == q;
        }
    };
}
#endif

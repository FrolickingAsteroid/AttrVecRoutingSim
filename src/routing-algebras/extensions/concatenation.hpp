#ifndef CONCATENATION_HPP
#define CONCATENATION_HPP

#include "routing-algebras/attributes/paths.hpp"

namespace RoutingAlgebras {
    template<class Node>
    struct Concatenation {
        Path<Node> operator()(const Path<Node> &p, const Node &entry) const {
            Path<Node> result = p;
            result.path.insert(result.path.begin(), entry);
            return result;
        }
    };
}
#endif

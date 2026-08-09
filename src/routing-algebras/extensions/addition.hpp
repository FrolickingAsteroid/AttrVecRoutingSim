#ifndef LENGTHS_EXTENSION_H
#define LENGTHS_EXTENSION_H

namespace RoutingAlgebras {
    typedef struct Addition {
        int operator()(const int l, const int r) const {
            const int sum = l + r;
            return sum;
        }
    } ADD;
} // namespace RoutingAlgebras

#endif

#ifndef GEQ_ORDER_H
#define GEQ_ORDER_H

#include <climits>

namespace RoutingAlgebras {
    typedef struct GreaterThanOrEqual {
        static constexpr int null = INT_MAX;
        static constexpr int neutral = 0;

        bool operator()(const int l, const int r) const {
            return l >= r;
        }

        static int Neutral() {
            return neutral;
        }

        static int Null() {
            return null;
        }
    } GEQ;
} // namespace RoutingAlgebras

#endif

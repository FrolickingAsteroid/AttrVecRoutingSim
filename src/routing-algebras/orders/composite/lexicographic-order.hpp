#ifndef LEXICOGRAPHIC_ORDER_HPP
#define LEXICOGRAPHIC_ORDER_HPP

#include "routing-algebras/orders/comparison-vocab.hpp"

namespace RoutingAlgebras {
    template<class OrdA, class OrdB>
    struct LexicographicOrder {
        template<class Attr>
        Order operator()(const Attr &l, const Attr &r) const {
            OrdA ord_a;

            bool aL = ord_a(l.a, r.a);
            bool aR = ord_a(r.a, l.a);

            if (aL && !aR) return Order::Better; // l is strictly better on ordA
            if (!aL && aR) return Order::Worse; // l strictly worse on ordA

            // can the first order ever return incomparable?

            // defer to B
            OrdB ord_b;
            bool bL = ord_b(l.b, r.b);
            bool bR = ord_b(r.b, l.b);

            if (bL && bR) return Order::Equal;
            if (bL) return Order::Better;
            if (bR) return Order::Worse;

            return Order::Incomparable;
        }
    };
}
#endif

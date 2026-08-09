#ifndef PRODUCT_ORDER_HPP
#define PRODUCT_ORDER_HPP

#include "routing-algebras/orders/comparison-vocab.hpp"

namespace RoutingAlgebras {
    template<class OrdA, class OrdB>
    struct ProductOrder {
        template<class Attr>
        Order operator()(const Attr &l, const Attr &r) const {
            OrdA ord_a;
            OrdB ord_b;

            bool lR = ord_a(l.a, r.a) && ord_b(l.b, r.b); // l <= r componentwise
            bool rL = ord_a(r.a, l.a) && ord_b(r.b, l.b); // r <= l componentwise

            if (lR && rL) return Order::Equal;
            if (lR) return Order::Better;
            if (rL) return Order::Worse;

            return Order::Incomparable;
        }
    };
}
#endif

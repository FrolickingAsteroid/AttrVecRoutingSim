#ifndef SEQ_EXTENSION_H
#define SEQ_EXTENSION_H

#include "routing-algebras/attributes/sequenced-attributes.hpp"

namespace RoutingAlgebras {
    template<class A, class ExtBase>
    struct SeqExtension {
        SeqAttr<A> operator()(const SeqAttr<A> &l, const SeqAttr<A> &r) const {
            ExtBase ext;

            SeqAttr<A> result;
            result.n = r.n; // Keep the sequence number from the new attr
            result.a = ext(l.a, r.a); // Extend base attribute
            return result;
        }
    };
} // namespace RoutingAlgebras

#endif

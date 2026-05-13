#ifndef SEQ_EXTENSION_H
#define SEQ_EXTENSION_H

#include <utility>
#include "base-attributes/extensions/lengths-ext.hpp"
#include "routing-algebras/sequenced-ra.hpp"

namespace RoutingAlgebras {
    template<class A, class ExtBase>
    struct SeqExtension {
        SeqAttr<A> operator()(const SeqAttr<A> &l, const SeqAttr<A> &r) const {
            ExtBase ext;

            // Keep the sequence number from the new attr
            // Extend base attribute
            SeqAttr<A> result;
            result.n = r.n;
            result.a = ext(l.a, r.a);
            return result;
        }
    };
} // namespace RoutingAlgebras

namespace RoutingAlgebras {
    // --- Sequenced Length Algebra --- //
    using ESL = SeqExtension<int, LE>;
    // ------------------------------- //
}

#endif// SEQ_EXTENSION_H

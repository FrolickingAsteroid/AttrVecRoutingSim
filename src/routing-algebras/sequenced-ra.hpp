#ifndef SEQUENCED_H
#define SEQUENCED_H

namespace RoutingAlgebras {
    // sequenced routing algebra
    template<class A>
    struct SeqAttr {
        int n{}; // Sequence number
        A a; // Base routing algebra attribute (n encapsulates base attribute A)

        SeqAttr() = default;

        SeqAttr(A a_) : n(0), a(a_) {
        }

        SeqAttr(int n_, A a_) : n(n_), a(a_) {
        }

        bool operator==(const SeqAttr<A> &other) const {
            return n == other.n && a == other.a;
        }

        bool operator!=(const SeqAttr<A> &other) const {
            return !(*this == other);
        }
    };
} // namespace RoutingAlgebras

namespace RoutingAlgebras {
    // --- Sequenced Length Algebra --- //
    using SL = SeqAttr<int>;
    // ------------------------------- //
}

#endif /* SEQUENCED_H */

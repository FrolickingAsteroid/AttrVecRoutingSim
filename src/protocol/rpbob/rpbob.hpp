#ifndef RPBOB_HPP
#define RPBOB_HPP

#include <vector>
#include "protocol/rpbob/state/path-segment.hpp"
#include "protocol/rpbob/state/seq-window.hpp"

#include "protocol/protocol.hpp"

namespace Protocol {
    template<class A, class Ord, class Ext>
    struct RPBOB :  IProtocol<A>{

        using IProtocol<A>::graph;
        using IProtocol<A>::cal;
        using IProtocol<A>::data;

        using path = State::Path<A>;
        using window = State::SequenceWindow<A, Ord>;
        using seq_attr = RoutingAlgebras::SeqAttr<A>;

        std::vector<window> s;

        explicit RPBOB(Graphs::Graph<A> &graph_, std::size_t N_)
            : IProtocol<A>(graph_),
              s(graph_.V, window(N_)) {}

        void MsgReception(int u, int v, path p);

        void MsgAdvertisement(int u, int v, double delay, path p);

        void LinkFailure(int u, int v);

        void DestinationAnnouncement(int t, int n, double period, int max);
    };
}// namespace Protocol

#include "protocol/rpbob/rpbob.tpp"

#endif

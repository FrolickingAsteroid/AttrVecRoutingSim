#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <vector>

#include "graph/graph.hpp"
#include "protocol/state/path-segment.hpp"
#include "protocol/state/seq-window.hpp"
#include "scheduler/calendar.hpp"
#include "utils/data.hpp"

namespace Protocol {
    template<class A, class Ord, class Ext>
    struct ProtocolGlobalState {
        using path = State::Path<A>;
        using window = State::SequenceWindow<A, Ord>;
        using seq_attr = RoutingAlgebras::SeqAttr<A>;

        Graphs::Graph<A> &graph;
        std::vector<window> s;
        scheduler::Calendar cal;

        utils::Data<A> data;

        explicit ProtocolGlobalState(Graphs::Graph<A> &graph_, std::size_t N_)
            : graph(graph_),
              s(graph_.V, window(N_)) {
        }

        void Run() {
            cal.run();
        }

        void MsgReception(int u, int v, path p);

        void MsgAdvertisement(int u, int v, double delay, path p);

        void LinkFailure(int u, int v);

        void DestinationAnnouncement(int t, int n, double period, int max);
    };
}// namespace Protocol

#include "protocol/protocol.tpp"

#endif

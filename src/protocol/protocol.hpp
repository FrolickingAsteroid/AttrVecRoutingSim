#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "scheduler/calendar.hpp"
#include "utils/data.hpp"
#include "graph/graph.hpp"

namespace Protocol {

    template<class A>
    struct IProtocol {
        Graphs::Graph<A> &graph;
        scheduler::Calendar cal;
        utils::Data<A> data;

        explicit IProtocol(Graphs::Graph<A> &graph_) : graph(graph_) {}
        virtual ~IProtocol() = default;

        void Run() { cal.run(); }

        virtual void LinkFailure(int u, int v) = 0;
        virtual void DestinationAnnouncement(int t, int n, double period, int max) = 0;
    };

}
#endif
#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "graph/graph.hpp"
#include "utils/stats/data.hpp"

#include "scheduler/rng.hpp"
#include "scheduler/calendar.hpp"


namespace Protocol {
    template<class A, class R, class Msg = R>
    struct IProtocol {
        Graphs::Graph<A> &graph;
        scheduler::Calendar cal;
        utils::stats::Data<R> data;

        explicit IProtocol(Graphs::Graph<A> &graph_)
            : graph(graph_),
              data(graph_.V) {
        }

        virtual ~IProtocol() = default;

        virtual void MsgReception(int u, int v, Msg msg) = 0;

        virtual void LinkFailure(int u, int v) = 0;

        virtual void process() {
        }

        void Run() {
            cal.run([this]() {
                process();
            });
        }

        void Send(int t, Msg msg) {
            for (auto &[r, edge]: graph.adjListIn[t]) {
                auto &[delay, active, attr] = edge;

                if (!active)
                    continue;

                const double rand_delay = rng_uniform(0.8 * delay, delay);

                cal.schedule(cal.now() + rand_delay, [this, r, t, msg]() mutable {
                    this->MsgReception(r, t, msg);
                });

                data.recordMessage(t, cal.now());
            }
        }
    };
}
#endif

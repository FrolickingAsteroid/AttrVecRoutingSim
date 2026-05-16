#ifndef RPBOB_SIMULATION_HPP
#define RPBOB_SIMULATION_HPP

#include "protocol/rpbob/rpbob.hpp"
#include "simulator/simulator.hpp"
#include "utils/logger.hpp"

#include <cstddef>
#include <string>
#include <numeric>

namespace Simulator {

    // simulator/rpbob-simulator.hpp
    template<class A, class Ord, class Ext>
    class RpBOBSimulation : public Simulation {
    public:
        RpBOBSimulation(const std::string &filename, std::size_t N_,
                        int dest_, int max_, double period_,
                        utils::RegisterType log_t_)
            : graph(ReadGraph<A>(filename)),
              registrar(filename, max_, dest_, log_t_),
              period(period_), dest(dest_), max(max_), N(N_) {}

        void setup() override {
            if (dest < 0) {
                targets.resize(graph.V);
                std::iota(targets.begin(), targets.end(), 0);
            } else {
                targets = {dest};
            }
        }

        void run() override {
            for (int t : targets) {
                Protocol::RPBOB<A,Ord,Ext> proto(graph, N);
                proto.DestinationAnnouncement(t, 0, period, max);
                proto.Run();
                registrar.RegisterDestination(t, proto.data);
            }
        }
    private:
        Graphs::Graph<A> graph;
        utils::Registrar<A> registrar;
        double period;
        int dest, max;
        std::size_t N;
        std::vector<int> targets;
    };

}// namespace Simulator

#endif
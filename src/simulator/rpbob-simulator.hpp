#ifndef RPBOB_SIMULATION_HPP
#define RPBOB_SIMULATION_HPP

#include "protocol/protocol.hpp"
#include "simulator/simulator.hpp"
#include "utils/logger.hpp"

#include <cstddef>
#include <string>

namespace Simulator {

    template<class A, class Ord, class Ext>
    class RpBOBSimulation : public Simulation {
    public:
        using protocol = Protocol::ProtocolGlobalState<A, Ord, Ext>;

        RpBOBSimulation(const std::string &filename_,
                        std::size_t N_,
                        int dest_,
                        int max_,
                        double period_,
                        utils::RegisterType log_t_)
            : filename(filename_),
              graph(ReadGraph<A>(filename_)),
              period(period_),
              dest(dest_),
              max(max_),
              N(N_),
              registrar(filename_, max_, dest_, log_t_) {
        }

        void setup() override {
            //pass
        }

        void run() override {
            if (dest < 0) {
                runAllDestinations();
            } else {
                runDestination(dest);
            }
        }

        void runDestination(int t) {
            l.log(utils::INFO, "Starting destination " + std::to_string(t));

            protocol proto(graph, N);

            proto.DestinationAnnouncement(t, 0, period, max);
            proto.Run();

            registrar.RegisterDestination(t, proto.data);

            l.log(utils::INFO, "Finished destination " + std::to_string(t));
        }

        void runAllDestinations() {
            for (int t = 0; t < graph.V; ++t) {
                runDestination(t);
            }
        }

    private:
        std::string filename;

        Graphs::Graph<A> graph;

        double period;
        int dest;
        int max;
        std::size_t N;

        utils::Logger l;
        utils::Registrar<A> registrar;
    };

}// namespace Simulator

#endif
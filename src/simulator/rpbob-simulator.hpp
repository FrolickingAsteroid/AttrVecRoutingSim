#ifndef RPBOB_SIMULATION_HPP
#define RPBOB_SIMULATION_HPP

#include "simulator/simulator.hpp"
#include "protocol/protocol.hpp"
#include "utils/logger.hpp"

#include <cstddef>
#include <fstream>
#include <string>


namespace Simulator {
    template<class A, class Ord, class Ext>
    class RpBOBSimulation : public Simulation {
    public:
        using protocol = Protocol::ProtocolGlobalState<A, Ord, Ext>;
        utils::Logger l;

        RpBOBSimulation(const std::string &filename_,
                        std::size_t N_,
                        int dest_,
                        int max_,
                        double period_) : filename(filename_),
                                          graph(ReadGraph<A>(filename_)),
                                          proto(graph, N_),
                                          period(period_),
                                          dest(dest_),
                                          max(max_),
                                          N(N_) {
        }

        void setup() override {
            proto.DestinationAnnouncement(
                dest, 0,
                period,
                max
            );
        }

        void run() override {
            setup();
            proto.cal.run();

            l.log(utils::INFO, "Finished simulation");
            proto.data.write_csv(filename, max, dest);
        }

    private:
        std::string filename;

        Graphs::Graph<A> graph;
        protocol proto;

        double period;
        int dest;
        int max;
        std::size_t N;
    };
} // namespace Simulator

#endif

#include "routing-algebras/sequenced-ext.hpp"
#include "routing-algebras/sequenced-ord.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

#include "simulator/rpbob-simulator.hpp"
#include "utils/logger.hpp"

using namespace RoutingAlgebras;

int main(int argc, char **argv) {
    if (argc != 7) {
        std::cerr << "usage: "
                << argv[0]
                << " <network-file> <protocol-kind> <destination> <N> <max-beacons> <period>\n";

        std::cerr << "example:\n"
                << "  " << argv[0] << " network.txt seq-length 0 5 100 1.0\n";

        return 1;
    }

    utils::Logger l;

    std::string filename = argv[1];
    std::string kind = argv[2];


    int destination = std::stoi(argv[3]);
    std::size_t N = static_cast<std::size_t>(std::stoul(argv[4]));
    int max_beacons = std::stoi(argv[5]);
    double period = std::stod(argv[6]);


    if (kind == "seq-length") {
        using A = int;
        using Ord = OSL;
        using Ext = ESL;

        Simulator::RpBOBSimulation<A, Ord, Ext> sim(
            filename,
            N,
            destination,
            max_beacons,
            period
        );
        l.log(utils::INFO, "Starting Simulation");
        sim.run();
    } else {
        std::cerr << "unknown protocol kind: " << kind << "\n";
        return 1;
    }


    return 0;
}

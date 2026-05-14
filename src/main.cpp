#include "routing-algebras/sequenced-ext.hpp"
#include "routing-algebras/sequenced-ord.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "simulator/rpbob-simulator.hpp"
#include "utils/logger.hpp"

using namespace RoutingAlgebras;

utils::RegisterType ParseR(const std::string &s) {
    if (s == "set") {
        return utils::RegisterType::Set;
    }

    if (s == "stab") {
        return utils::RegisterType::NodeStab;
    }

    throw std::runtime_error("unknown Register type: " + s);
}

int main(int argc, char **argv) {
    if (argc != 8) {
        std::cerr << "usage: "
                  << argv[0]
                  << " <network-file> <protocol-kind> <destination|all> <N> <max-beacons> <period> <reg-mode>\n";

        std::cerr << "example:\n"
                  << "  " << argv[0] << " network.txt seq-length all 5 100 1.0\n";

        return 1;
    }

    utils::Logger l;

    std::string filename = argv[1];
    std::string kind = argv[2];
    std::string destination_arg = argv[3];

    std::size_t N = std::stoul(argv[4]);
    int max_beacons = std::stoi(argv[5]);
    double period = std::stod(argv[6]);

    utils::RegisterType reg = ParseR(argv[7]);

    try {
        if (kind == "seq-length") {
            using A = int;
            using Ord = OSL;
            using Ext = ESL;

            int destination;

            if (destination_arg == "all") {
                destination = -1;
            } else {
                destination = std::stoi(destination_arg);
            }

            Simulator::RpBOBSimulation<A, Ord, Ext> sim(
                    filename,
                    N,
                    destination,
                    max_beacons,
                    period,
                    reg);

            sim.run();
        } else {
            std::cerr << "unknown protocol kind: " << kind << "\n";
            return 1;
        }
    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
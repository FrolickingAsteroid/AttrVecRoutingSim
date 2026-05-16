#include "routing-algebras/sequenced-ext.hpp"
#include "routing-algebras/sequenced-ord.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <memory>

#include "simulator/simulator.hpp"
#include "simulator/rpbob-simulator.hpp"
#include "utils/data.hpp"


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

std::unique_ptr<Simulator::Simulation> make_simulation(
    const std::string &kind,
    const std::string &filename,
    std::size_t N, int dest, int max,
    double period, utils::RegisterType reg)
{
    if (kind == "seq-length") {
        return std::make_unique<
            Simulator::RpBOBSimulation<int, OSL, ESL>>(
                filename, N, dest, max, period, reg);
    }
    throw std::runtime_error("unknown protocol kind: " + kind);
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
    int max = std::stoi(argv[5]);
    double period = std::stod(argv[6]);
    utils::RegisterType reg = ParseR(argv[7]);

    int dest;
    if (destination_arg == "all") {
        dest = -1;
    } else {
        dest= std::stoi(destination_arg);
    }

    auto sim = make_simulation(kind, filename, N, dest, max, period, reg);
    sim->setup();
    sim->run();

    return 0;
}
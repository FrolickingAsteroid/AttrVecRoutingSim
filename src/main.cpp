#include "routing-algebras/sequenced-ext.hpp"
#include "routing-algebras/sequenced-ord.hpp"

#include "simulator/becing-simulator.hpp"
#include "simulator/rpbob-simulator.hpp"
#include "simulator/simulator.hpp"
#include "utils/data.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace RoutingAlgebras;

namespace
{

struct SimulationConfig {
	std::string network_file;
	std::string protocol_kind;
	int destination;
	std::size_t window_size;
	int max_beacons;
	double period;
	utils::RegisterType register_type;
};

void PrintUsage(const char *program)
{
	std::cerr
		<< "usage:\n"
		<< "  " << program
		<< " <path/to/network-file> <protocol> <destination|all> <N> "
		   "<max-beacons> <period> <reg>\n\n"

		<< "protocol:\n"
		<< "  seq-length      RPBOB / sequenced length protocol\n"
		<< "  beacon-length   Beaconing baseline with length metric\n\n"

		<< "destination:\n"
		<< "  all             run for all destinations\n"
		<< "  <node-id>        run for one destination\n\n"

		<< "reg:\n"
		<< "  set             record path set each time it changes\n"
		<< "  stab            record node stabilization times\n\n"

		<< "examples:\n"
		<< "  " << program
		<< " network.tsv seq-length all 5 100 5.0 set\n"
		<< "  " << program
		<< " network.tsv seq-length 0 5 100 5.0 stab\n"
		<< "  " << program
		<< " network.tsv beacon-length all 0 100 5.0 stab\n";
}

utils::RegisterType ParseRegisterType(const std::string &s)
{
	if (s == "set") {
		return utils::RegisterType::Set;
	}
	if (s == "stab") {
		return utils::RegisterType::NodeStab;
	}
	throw std::runtime_error("unknown register mode: " + s);
}

int ParseDestination(const std::string &s)
{
	if (s == "all") {
		return -1;
	}
	return std::stoi(s);
}

SimulationConfig ParseArgs(int argc, char **argv)
{
	if (argc != 8) {
		throw std::runtime_error("invalid number of arguments");
	}

	SimulationConfig cfg;
	cfg.network_file = argv[1];
	cfg.protocol_kind = argv[2];
	cfg.destination = ParseDestination(argv[3]);
	cfg.window_size = std::stoul(argv[4]);
	cfg.max_beacons = std::stoi(argv[5]);
	cfg.period = std::stod(argv[6]);
	cfg.register_type = ParseRegisterType(argv[7]);

	if (cfg.max_beacons <= 0) {
		throw std::runtime_error("max-beacons must be positive");
	}
	if (cfg.period <= 0.0) {
		throw std::runtime_error("period must be positive");
	}

	return cfg;
}

std::unique_ptr<Simulator::ISimulation>
MakeSeqLengthSimulation(const SimulationConfig &cfg)
{
	return std::make_unique<Simulator::RpBOBSimulation<int, OSL, ESL> >(
		cfg.network_file, cfg.window_size, cfg.max_beacons,
		cfg.destination, cfg.period, cfg.register_type);
}

std::unique_ptr<Simulator::ISimulation>
MakeBeaconLengthSimulation(const SimulationConfig &cfg)
{
	return std::make_unique<Simulator::BECINGSimulation<int, TOL, LE> >(
		cfg.network_file, cfg.max_beacons, cfg.destination, cfg.period,
		cfg.register_type);
}

std::unique_ptr<Simulator::ISimulation>
MakeSimulation(const SimulationConfig &cfg)
{
	if (cfg.protocol_kind == "seq-length") {
		return MakeSeqLengthSimulation(cfg);
	}
	if (cfg.protocol_kind == "beacon-length") {
		return MakeBeaconLengthSimulation(cfg);
	}

	throw std::runtime_error("unknown protocol kind: " + cfg.protocol_kind);
}

void RunSimulation(const SimulationConfig &cfg)
{
	auto sim = MakeSimulation(cfg);
	sim->setup();
	sim->run();
}

} // namespace

int main(int argc, char **argv)
{
	const SimulationConfig cfg = ParseArgs(argc, argv);
	RunSimulation(cfg);
	return 0;
}
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

#include <argparse/argparse.hpp>

using namespace RoutingAlgebras;

namespace
{

namespace term
{
constexpr const char *bold = "\033[1m";
constexpr const char *cyan = "\033[36m";
constexpr const char *yellow = "\033[33m";
constexpr const char *reset = "\033[0m";
}

struct SimulationConfig {
	std::string network_file;
	std::string protocol;
	int destination;

	std::size_t window_size;
	int max_beacons;

	double period;
	double dprop;

	utils::RegisterType register_type;
};

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
		cfg.network_file, cfg.max_beacons, cfg.destination, cfg.dprop,
		cfg.period, cfg.register_type);
}

std::unique_ptr<Simulator::ISimulation>
MakeSimulation(const SimulationConfig &cfg)
{
	if (cfg.protocol == "seq-length") {
		return MakeSeqLengthSimulation(cfg);
	}
	if (cfg.protocol == "beacon-length") {
		return MakeBeaconLengthSimulation(cfg);
	}
	throw std::runtime_error("unknown protocol: " + cfg.protocol);
}

void RunSimulation(const SimulationConfig &cfg)
{
	auto sim = MakeSimulation(cfg);
	sim->setup();
	sim->run();
}

utils::RegisterType ParseRegisterType(const std::string &s)
{
	if (s == "set") {
		return utils::RegisterType::Set;
	}
	if (s == "stab") {
		return utils::RegisterType::NodeStab;
	}
	throw std::runtime_error("unknown data collection: " + s);
}

int ParseDest(const std::string &s)
{
	if (s == "all") {
		return -1;
	}
	return std::stoi(s);
}

SimulationConfig ParseArgs(int argc, const char **argv)
{
	SimulationConfig cfg;
	argparse::ArgumentParser parser(argc, argv);

	parser.add_argument("--network_file")
		.set_metavar("network.tsv")
		.set_type(argparse::ArgTypes::STRING)
		.set_flags(argparse::ArgFlags::REQUIRED)
		.set_help("Path to the network file");

	parser.add_argument("--protocol")
		.set_metavar("proto")
		.set_type(argparse::ArgTypes::STRING)
		.set_flags(argparse::ArgFlags::REQUIRED)
		.set_help(std::string(term::bold) + "Protocol to run: " +
			  term::reset + term::cyan + "rpbob" + term::reset +
			  ", " + term::cyan + "beaconing" + term::reset + ", " +
			  term::cyan + "bgp\n" + term::reset);

	parser.add_argument("--destination")
		.set_metavar("all/id")
		.set_type(argparse::ArgTypes::STRING)
		.set_flags(argparse::ArgFlags::REQUIRED)
		.set_help(
			std::string(term::bold) +
			"node that will announce itself as the destination:\n" +
			term::reset + term::yellow + "\t\t - node id\n" +
			"\t\t - all\n" + term::reset);

	parser.add_argument("--max")
		.set_metavar("m")
		.set_type(argparse::ArgTypes::INT)
		.set_flags(argparse::ArgFlags::NONE)
		.set_help(
			std::string(term::bold) +
			"For rpbob/beaconing: " + term::reset +
			"Maximum number of periodic destination announcements\n");

	parser.add_argument("--param")
		.set_metavar("N/delay prop")
		.set_type(argparse::ArgTypes::STRING)
		.set_flags(argparse::ArgFlags::NONE)
		.set_help(std::string(term::bold) +
			  "Protocol-dependent parameter:\n" + term::reset +
			  term::yellow + "\t\t - rpbob: " + term::reset +
			  "window size " + term::cyan + "N\n" + term::reset +
			  term::yellow + "\t\t - beaconing: " + term::reset +
			  "propagation delay " + term::cyan + "d_prop\n" +
			  term::reset);

	parser.add_argument("--period")
		.set_metavar("delta")
		.set_type(argparse::ArgTypes::STRING)
		.set_flags(argparse::ArgFlags::NONE)
		.set_help(std::string(term::bold) +
			  "For rpbob/beaconing: " + term::reset +
			  "periodicity of destination announcements\n");

	parser.add_argument("--reg")
		.set_metavar("data")
		.set_type(argparse::ArgTypes::STRING)
		.set_flags(argparse::ArgFlags::REQUIRED)
		.set_help(std::string(term::bold) +
			  "Type of Data to be collected:\n" + term::reset +
			  +term::yellow + "\t\t - stab: " + term::reset +
			  "stabilization times of each node\n" + +term::reset +
			  term::yellow + "\t\t - set: " + term::reset +
			  "evolution of the set of paths at each node\n" +
			  term::reset);

	try {
		auto args = parser.parse_args();

		cfg.network_file = args.at("--network_file").as<std::string>();
		cfg.protocol = args.at("--protocol").as<std::string>();
		cfg.destination =
			ParseDest(args.at("--destination").as<std::string>());

		cfg.max_beacons = args.at("--max").as<int>();
		cfg.period = std::stod(args.at("--period").as<std::string>());
		cfg.register_type =
			ParseRegisterType(args.at("--reg").as<std::string>());

		const auto param = args.at("--param").as<std::string>();

		if (cfg.protocol == "beacon-length") {
			cfg.window_size = 0;
			cfg.dprop = std::stod(param);
		} else if (cfg.protocol == "rpbob" ||
			   cfg.protocol == "seq-length") {
			cfg.window_size = std::stoul(param);
			cfg.dprop = 0.0;
		}
	} catch (const std::runtime_error &err) {
		std::cerr << err.what() << std::endl;
		std::exit(1);
	}

	return cfg;
}

}

int main(int argc, const char **argv)
{
	const SimulationConfig cfg = ParseArgs(argc, argv);
	RunSimulation(cfg);
	return 0;
}
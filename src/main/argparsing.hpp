#ifndef ARGPARSING_HPP
#define ARGPARSING_HPP

#include "routing-algebras/sequenced-ord.hpp"
#include "routing-algebras/sequenced-ext.hpp"

#include "simulator/simulator.hpp"
#include "simulator/becing-simulator.hpp"
#include "simulator/rpbob-simulator.hpp"

#include "utils/ui/helpers.hpp"
#include "utils/register/data.hpp"

#include <cstdlib>
#include <memory>
#include <string>

#include <stdexcept>
#include <iostream>

using namespace RoutingAlgebras;
using namespace std;
using namespace utils::ui;

using Args = std::unordered_map<std::string, std::string>;

namespace config
{

struct SimulationConfig {
	// required args
	std::string network_file;
	std::string protocol;
	int destination;

	// optional args
	optional<double> dprop = nullopt;
	optional<double> period = nullopt;
	optional<int> max_beacons = nullopt;
	optional<std::size_t> window_size = nullopt;

	// data collection type
	utils::RegisterType register_type;
};

template <class A, class Ord, class Ext>
std::unique_ptr<Simulator::ISimulation>
RPBOBSimulation(const SimulationConfig &cfg)
{
	return std::make_unique<Simulator::RpBOBSimulation<A, Ord, Ext> >(
		cfg.network_file, cfg.window_size.value(),
		cfg.max_beacons.value(), cfg.destination, cfg.period.value(),
		cfg.register_type);
}

template <class A, class Ord, class Ext>
std::unique_ptr<Simulator::ISimulation>
BeaconingSimulation(const SimulationConfig &cfg)
{
	return std::make_unique<Simulator::BECINGSimulation<A, Ord, Ext> >(
		cfg.network_file, cfg.max_beacons.value(), cfg.destination,
		cfg.dprop.value(), cfg.period.value(), cfg.register_type);
}

inline std::unique_ptr<Simulator::ISimulation>
MakeSimulation(const SimulationConfig &cfg)
{
	if (cfg.protocol == "rpbob-length") {
		return RPBOBSimulation<int, OSL, ESL>(cfg);
	}

	if (cfg.protocol == "beacon-length") {
		return BeaconingSimulation<int, TOL, LE>(cfg);
	}

	throw std::runtime_error("unknown protocol: " + cfg.protocol);
}

inline utils::RegisterType ParseRegisterType(const std::string &s)
{
	if (s == "set") {
		return utils::RegisterType::Set;
	}
	if (s == "stab") {
		return utils::RegisterType::NodeStab;
	}
	throw std::runtime_error("unknown data collection: " + s);
}

inline int ParseDest(const std::string &s)
{
	if (s == "all") {
		return -1;
	}
	return std::stoi(s);
}

inline void PrintUsage(const char *prog)
{
	std::cerr
		<< yellow("usage:\n") << "  " << prog
		<< " --network_file <network.tsv>"
		<< " --protocol <rpbob-length|beacon-length>"
		<< " --destination <id|all>"
		<< " --max <m>"
		<< " --param <N|dprop>"
		<< " --period <delta>"
		<< " --reg <stab|set>\n\n"

		<< yellow("arguments:\n")
		<< "  --network_file   path to the network file\n"
		<< "  --protocol       rpbob-length or beacon-length\n"
		<< "  --destination    node id or all\n"
		<< "  --max            maximum number of destination announcements\n"
		<< "  --param          N for rpbob-length; dprop for beacon-length\n"
		<< "  --period         destination announcement period\n"
		<< "  --reg            data to collect: stab or set\n\n";
}

inline Args ParseNamedArgs(int argc, const char **argv)
{
	Args args;

	for (int i = 1; i < argc; ++i) {
		std::string key = argv[i];

		if (key == "-h" || key == "--help") {
			PrintUsage(argv[0]);
			std::exit(0);
		}

		if (key.rfind("--", 0) != 0) {
			PrintUsage(argv[0]);
			throw std::runtime_error(key);
		}

		if (i + 1 >= argc) {
			PrintUsage(argv[0]);
			throw std::runtime_error("missing key for" + key);
		}

		std::string value = argv[++i];

		if (value.rfind("--", 0) == 0) {
			PrintUsage(argv[0]);
			throw std::runtime_error("missing value for " + key);
		}

		args.emplace(std::move(key), std::move(value));
	}

	return args;
}

inline std::string Require(const Args &args, const std::string &key)
{
	auto it = args.find(key);

	if (it == args.end())
		throw std::runtime_error("missing argument: " + key);

	return it->second;
}

inline SimulationConfig ParseArgs(int argc, const char **argv)
{
	auto args = ParseNamedArgs(argc, argv);

	SimulationConfig cfg;

	cfg.network_file = Require(args, "--network_file");
	cfg.protocol = Require(args, "--protocol");
	cfg.destination = ParseDest(Require(args, "--destination"));
	cfg.register_type = ParseRegisterType(Require(args, "--reg"));

	if (cfg.protocol.find("rpbob") != std::string::npos) {
		cfg.max_beacons = std::stoi(Require(args, "--max"));
		cfg.period = std::stod(Require(args, "--period"));
		cfg.window_size = std::stoul(Require(args, "--param"));
	} else if (cfg.protocol.find("beacon") != std::string::npos) {
		cfg.max_beacons = std::stoi(Require(args, "--max"));
		cfg.period = std::stod(Require(args, "--period"));
		cfg.dprop = std::stod(Require(args, "--param"));
	} else {
		throw std::runtime_error("unknown protocol: " + cfg.protocol);
	}

	return cfg;
}

}

#endif
#include "routing-algebras/sequenced-ext.hpp"
#include "routing-algebras/sequenced-ord.hpp"

#include "simulator/becing-simulator.hpp"
#include "simulator/rpbob-simulator.hpp"
#include "simulator/simulator.hpp"
#include "utils/ui/helpers.hpp"
#include "utils/register/data.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include <argparse/argparse.hpp>

using namespace RoutingAlgebras;
using namespace std;
using namespace utils::ui;

namespace
{

struct SimulationConfig {
	std::string network_file;
	std::string protocol;
	int destination;

	optional<std::size_t> window_size = nullopt;
	optional<double> dprop = nullopt;

	int max_beacons;
	double period;

	utils::RegisterType register_type;
};

std::unique_ptr<Simulator::ISimulation>
MakeSeqLengthSimulation(const SimulationConfig &cfg)
{
	return std::make_unique<Simulator::RpBOBSimulation<int, OSL, ESL> >(
		cfg.network_file, cfg.window_size.value(), cfg.max_beacons,
		cfg.destination, cfg.period, cfg.register_type);
}

std::unique_ptr<Simulator::ISimulation>
MakeBeaconLengthSimulation(const SimulationConfig &cfg)
{
	return std::make_unique<Simulator::BECINGSimulation<int, TOL, LE> >(
		cfg.network_file, cfg.max_beacons, cfg.destination,
		cfg.dprop.value(), cfg.period, cfg.register_type);
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
	using argparse::ArgFlags;
	using argparse::ArgTypes;

	SimulationConfig cfg;
	argparse::ArgumentParser parser(argc, argv);

	const auto req = ArgFlags::REQUIRED;
	const auto opt = ArgFlags::NONE;

	auto add_string = [&](const char *name, const char *meta,
			      ArgFlags flags, const std::string &help) {
		parser.add_argument(name)
			.set_metavar(meta)
			.set_type(ArgTypes::STRING)
			.set_flags(flags)
			.set_help(help);
	};

	auto add_int = [&](const char *name, const char *meta, ArgFlags flags,
			   const std::string &help) {
		parser.add_argument(name)
			.set_metavar(meta)
			.set_type(ArgTypes::INT)
			.set_flags(flags)
			.set_help(help);
	};

	add_string("--network_file", "network.tsv", req,
		   "Path to the network file");

	add_string("--protocol", "proto", req,
		   bold("Protocol to run: ") + cyan("seq-length") + ", " +
			   cyan("beacon-length") + ", " + cyan("bgp") + "\n");

	add_string("--destination", "all/id", req,
		   bold("Node that announces itself as destination:\n") +
			   yellow("\t\t - node id\n") + yellow("\t\t - all\n"));

	add_int("--max", "m", opt,
		bold("For seq-length/beacon-length: ") +
			"maximum number of destination announcements\n");

	add_string("--param", "N/dprop", opt,
		   bold("Protocol-dependent parameter:\n") +
			   yellow("\t\t - seq-length: ") + "window size " +
			   cyan("N\n") + yellow("\t\t - beacon-length: ") +
			   "propagation delay " + cyan("d_prop\n"));

	add_string("--period", "delta", opt,
		   bold("For seq-length/beacon-length: ") +
			   "period of destination announcements\n");

	add_string("--reg", "data", req,
		   bold("Type of data to collect:\n") +
			   yellow("\t\t - stab: ") +
			   "stabilization times of each node\n" +
			   yellow("\t\t - set: ") +
			   "evolution of the set of paths at each node\n");

	try {
		auto args = parser.parse_args();

		auto get = [&](const std::string &name) {
			return args.at(name).as<std::string>();
		};

		auto require = [&](const std::string &name) {
			if (args.find(name) == args.end())
				throw std::runtime_error("missing argument: " +
							 name);
			return get(name);
		};

		cfg.network_file = get("--network_file");
		cfg.protocol = get("--protocol");
		cfg.destination = ParseDest(get("--destination"));
		cfg.register_type = ParseRegisterType(get("--reg"));

		if (cfg.protocol == "seq-length") {
			cfg.max_beacons = args.at("--max").as<int>();
			cfg.period = std::stod(require("--period"));
			cfg.window_size = std::stoul(require("--param"));
		} else if (cfg.protocol == "beacon-length") {
			cfg.max_beacons = args.at("--max").as<int>();
			cfg.period = std::stod(require("--period"));
			cfg.dprop = std::stod(require("--param"));
		} else if (cfg.protocol == "bgp") {
			cfg.max_beacons = 0;
			cfg.period = 0.0;
			cfg.window_size = 0;
			cfg.dprop = 0.0;
		} else {
			throw std::runtime_error("unknown protocol: " +
						 cfg.protocol);
		}

		if ((cfg.protocol == "seq-length" ||
		     cfg.protocol == "beacon-length") &&
		    cfg.max_beacons <= 0)
			throw std::runtime_error("--max must be positive");

		if ((cfg.protocol == "seq-length" ||
		     cfg.protocol == "beacon-length") &&
		    cfg.period <= 0.0)
			throw std::runtime_error("--period must be positive");

		if (cfg.protocol == "seq-length" && cfg.window_size == 0)
			throw std::runtime_error("--param/N must be positive");

		if (cfg.protocol == "beacon-length" && cfg.dprop <= 0.0)
			throw std::runtime_error(
				"--param/dprop must be positive");

		return cfg;
	} catch (const std::exception &err) {
		std::cerr << err.what() << '\n';
		std::exit(1);
	}
}

}

int main(int argc, const char **argv)
{
	const SimulationConfig cfg = ParseArgs(argc, argv);
	RunSimulation(cfg);
	return 0;
}
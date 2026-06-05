#ifndef ARGPARSING_HPP
#define ARGPARSING_HPP

#include "routing-algebras/seq.hpp"
#include "routing-algebras/lengths.hpp"

#include "simulator/simulator.hpp"
#include "simulator/becing-simulator.hpp"
#include "simulator/rpbob-simulator.hpp"
#include "utils/register/data.hpp"

namespace config
{
enum Proto { RPBOB, Beaconing, BGP, N };
enum Metric { LENGTHS, HOPS };

struct SimulationConfig {
	// required args
	std::string network_file;
	Proto protocol = RPBOB;
	Metric algebra = LENGTHS;
	int destination = -1;

	// optional args
	optional<double> dprop = std::nullopt;
	optional<double> period = std::nullopt;
	optional<int> max_beacons = std::nullopt;
	optional<std::size_t> window_size = std::nullopt;

	// data collection
	utils::RegisterType register_type = utils::RegisterType::NodeStab;
};

// ---------------------------------------------------------------------------- *
//
// Helper print functions
//
// ---------------------------------------------------------------------------- *

void PrintUsage(const char *prog);
void PrintSummary(std::chrono::milliseconds elapsed);

// ---------------------------------------------------------------------------- *
//
// Helper parsing functions
//
// ---------------------------------------------------------------------------- *

utils::RegisterType ParseRegisterType(const std::string &s);
Metric ParseAlgebra(const std::string &s);
int ParseDest(const std::string &s);

SimulationConfig ParseArgs(int argc, char *const *argv);

// ---------------------------------------------------------------------------- *
//
// Simulator Factory design pattern
//
// ---------------------------------------------------------------------------- *

template <class RA>
std::unique_ptr<Simulator::ISimulation> RPBOBSimulation(const SimulationConfig &cfg)
{
	return std::make_unique<Simulator::RpBOBSimulation<RA>>(
		cfg.network_file, cfg.window_size.value(), cfg.max_beacons.value(), cfg.destination,
		cfg.period.value(), cfg.register_type);
}

template <class RA>
std::unique_ptr<Simulator::ISimulation> BeaconingSimulation(const SimulationConfig &cfg)
{
	return std::make_unique<Simulator::BECINGSimulation<RA>>(
		cfg.network_file, cfg.max_beacons.value(), cfg.destination, cfg.dprop.value(),
		cfg.period.value(), cfg.register_type);
}

inline std::unique_ptr<Simulator::ISimulation> MakeSimulation(const SimulationConfig &cfg)
{
	if (cfg.protocol == RPBOB) {
		switch (cfg.algebra) {
		case LENGTHS:
		case HOPS:
			return RPBOBSimulation<Sequenced<L>>(cfg);
		}
	} else if (cfg.protocol == Beaconing) {
		switch (cfg.algebra) {
		case LENGTHS:
		case HOPS:
			return BeaconingSimulation<L>(cfg);
		}
	}

	throw std::runtime_error("unknown protocol");
}

}
#endif

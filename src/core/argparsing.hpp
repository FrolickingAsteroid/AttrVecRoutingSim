#ifndef ARGPARSING_HPP
#define ARGPARSING_HPP

#include "routing-algebras/lengths.hpp"
#include "routing-algebras/seq.hpp"

#include "simulator/becing-simulator.hpp"
#include "simulator/bgp-simulator.hpp"
#include "simulator/rpbob-simulator.hpp"
#include "simulator/simulator.hpp"

#include <map>
#include <memory>
#include <string>

namespace config
{

enum Options { RPBOB, Beaconing, BGP, N };
enum Metric { LENGTHS, HOPS };

struct SimulationConfig {
	// required args
	std::string network_file;
	Options	    protocol	= N;
	Metric	    algebra	= LENGTHS;
	int	    destination = -1;

	// optional args
	std::optional<double>	   dprop       = std::nullopt;
	std::optional<double>	   period      = std::nullopt;
	std::optional<int>	   max_beacons = std::nullopt;
	std::optional<std::size_t> window_size = std::nullopt;
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

SimulationConfig ParseArgs(int argc, char *const *argv);

Metric ParseAlgebra(const std::string &s);
int    ParseDest(const std::string &s);

// ---------------------------------------------------------------------------- *
//
// Parameter map builders
//
// ---------------------------------------------------------------------------- *

inline std::string AlgebraName(Metric m)
{
	switch (m) {
	case LENGTHS:
		return "lengths";
	case HOPS:
		return "hops";
	default:
		throw std::runtime_error("unknown algebra");
	}
}

inline std::map<std::string, std::string> RPBOBParams(const SimulationConfig &cfg)
{
	return {
		{ "", AlgebraName(cfg.algebra) },
		{ "N", std::to_string(cfg.window_size.value()) },
		{ "period", utils::ui::formatParam(cfg.period.value()) },
	};
}

inline std::map<std::string, std::string> BeaconingParams(const SimulationConfig &cfg)
{
	return {
		{ "", AlgebraName(cfg.algebra) },
		{ "dprop", utils::ui::formatParam(cfg.dprop.value()) },
		{ "period", utils::ui::formatParam(cfg.period.value()) },
	};
}

inline std::map<std::string, std::string> BGPParams(const SimulationConfig &cfg)
{
	return {
		{ "", AlgebraName(cfg.algebra) },
	};
}

// ---------------------------------------------------------------------------- *
//
// Simulator Factory
//
// ---------------------------------------------------------------------------- *

template <class RA>
std::unique_ptr<Simulator::ISimulation> RPBOBSimulation(const SimulationConfig &cfg)
{
	return std::make_unique<Simulator::RpBOBSimulation<RA>>(cfg.network_file,
								RPBOBParams(cfg),
								cfg.window_size.value(),
								cfg.max_beacons.value(),
								cfg.destination,
								cfg.period.value());
}

template <class RA>
std::unique_ptr<Simulator::ISimulation> BeaconingSimulation(const SimulationConfig &cfg)
{
	return std::make_unique<Simulator::BECINGSimulation<RA>>(cfg.network_file,
								 BeaconingParams(cfg),
								 cfg.max_beacons.value(),
								 cfg.destination,
								 cfg.dprop.value(),
								 cfg.period.value());
}

template <class RA>
std::unique_ptr<Simulator::ISimulation> BGPSimulation(const SimulationConfig &cfg)
{
	return std::make_unique<Simulator::BGPSimulation<RA>>(
		cfg.network_file, BGPParams(cfg), cfg.destination);
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
	} else if (cfg.protocol == BGP) {
		switch (cfg.algebra) {
		case LENGTHS:
		case HOPS:
			return BGPSimulation<L>(cfg);
		}
	}

	throw std::runtime_error("unknown protocol");
}

} // namespace config
#endif
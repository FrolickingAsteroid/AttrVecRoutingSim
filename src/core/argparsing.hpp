#ifndef ARGPARSING_HPP
#define ARGPARSING_HPP

#include <map>
#include <memory>
#include <string>

#include "routing-algebras/lengths.hpp"
#include "routing-algebras/seq.hpp"

#include "simulator/becing-simulator.hpp"
#include "simulator/bgp-simulator.hpp"
#include "simulator/rpbob-simulator.hpp"

#include "core/config.hpp"

namespace config
{
// ---------------------------------------------------------------------------- *
//
// Helper print functions
//
// ---------------------------------------------------------------------------- *

void PrintUsage(const char *prog);
void PrintSummary(std::chrono::milliseconds elapsed, const utils::stats::SummaryStats &stats);

// ---------------------------------------------------------------------------- *
//
// Helper parsing functions
//
// ---------------------------------------------------------------------------- *

SimulationConfig ParseArgs(int argc, char *const *argv);
Metric		 ParseAlgebra(const std::string &s);
int		 ParseDest(const std::string &s);

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
		{ "N", std::to_string(cfg.N.value()) },
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

template <class RA> std::unique_ptr<Simulator::ISimulation> RPBOBSimulation(SimulationConfig &cfg)
{
	return std::make_unique<Simulator::RpBOBSimulation<RA>>(cfg, RPBOBParams(cfg));
}

template <class RA>
std::unique_ptr<Simulator::ISimulation> BeaconingSimulation(const SimulationConfig &cfg)
{
	return std::make_unique<Simulator::BECINGSimulation<RA>>(cfg.network_file,
								 BeaconingParams(cfg),
								 cfg.max.value(),
								 cfg.destination,
								 cfg.dprop.value(),
								 cfg.period.value(),
								 cfg.scenario);
}

template <class RA>
std::unique_ptr<Simulator::ISimulation> BGPSimulation(const SimulationConfig &cfg)
{
	return std::make_unique<Simulator::BGPSimulation<RA>>(
		cfg.network_file, BGPParams(cfg), cfg.destination, cfg.scenario);
}

inline std::unique_ptr<Simulator::ISimulation> MakeSimulation(SimulationConfig cfg)
{
	if (cfg.protocol == Options::RPBOB) {
		switch (cfg.algebra) {
		case LENGTHS:
		case HOPS:
			return RPBOBSimulation<Sequenced<L>>(cfg);
		}
	} else if (cfg.protocol == Options::Beaconing) {
		switch (cfg.algebra) {
		case LENGTHS:
		case HOPS:
			return BeaconingSimulation<L>(cfg);
		}
	} else if (cfg.protocol == Options::BGP) {
		switch (cfg.algebra) {
		case LENGTHS:
		case HOPS:
			return BGPSimulation<L>(cfg);
		}
	}

	throw std::runtime_error("unknown protocol");
}
}

#endif
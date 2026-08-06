#ifndef ARGPARSING_HPP
#define ARGPARSING_HPP

#include <map>
#include <memory>
#include <string>

#include "core/config.hpp"
#include "routing-algebras/lengths.hpp"
#include "routing-algebras/seq.hpp"
#include "simulator/instances/becing-simulator.hpp"
#include "simulator/instances/bgp-simulator.hpp"
#include "simulator/instances/rcn-simulator.hpp"
#include "simulator/instances/rpbob-simulator.hpp"

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

Metric ParseAlgebra(const std::string &s);

int ParseDest(const std::string &s);

// ---------------------------------------------------------------------------- *
//
// Parameter map builders
//
// ---------------------------------------------------------------------------- *

inline std::string ScenarioName(ScenarioType m)
{
	switch (m)
	{
	case Normal:
		return "normal";
	case LinkFailure:
		return "linkFailure";
	case Withdrawal:
		return "withdrawal";
	default:
		throw std::runtime_error("unknown scenario");
	}
}

inline std::string AlgebraName(Metric m)
{
	switch (m)
	{
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
	    {"notif",   std::to_string(cfg.notif)                                       },
	    {"append",  std::to_string(cfg.append)                                      },
	    {"A_",      AlgebraName(cfg.algebra)                                        },
	    {"N_",      cfg.N.has_value() ? std::to_string(cfg.N.value()) : "adaptative"},
	    {"period_", formatParam(cfg.period.value())                                 },
	    {"S_",      ScenarioName(cfg.scenario)                                      },
	};
}

inline std::map<std::string, std::string> BeaconingParams(const SimulationConfig &cfg)
{
	return {
	    {"append",  std::to_string(cfg.append)     },
            {"A_",      AlgebraName(cfg.algebra)       },
	    {"dprop_",  formatParam(cfg.dprop.value()) },
            {"period_", formatParam(cfg.period.value())},
	    {"S_",      ScenarioName(cfg.scenario)     },
	};
}

inline std::map<std::string, std::string> BGPParams(const SimulationConfig &cfg)
{
	return {
	    {"append", std::to_string(cfg.append)},
	    {"A_",     AlgebraName(cfg.algebra)  },
	    {"S_",     ScenarioName(cfg.scenario)},
	};
}

// ---------------------------------------------------------------------------- *
//
// Simulator Factory
//
// ---------------------------------------------------------------------------- *

template <class RA>
std::unique_ptr<Simulator::ISimulation> RPBOBSimulation(SimulationConfig &cfg)
{
	return std::make_unique<Simulator::RpBOBSimulation<RA>>(cfg, RPBOBParams(cfg), "rpbob");
}

template <class RA>
std::unique_ptr<Simulator::ISimulation> BeaconingSimulation(SimulationConfig &cfg)
{
	return std::make_unique<Simulator::BECINGSimulation<RA>>(cfg, BeaconingParams(cfg));
}

template <class RA>
std::unique_ptr<Simulator::ISimulation> BGPSimulation(SimulationConfig &cfg)
{
	return std::make_unique<Simulator::BGPSimulation<RA>>(cfg, BGPParams(cfg), "bgp");
}

template <class RA>
std::unique_ptr<Simulator::ISimulation> RCNSimulation(SimulationConfig &cfg)
{
	return std::make_unique<Simulator::RCNSimulation<RA>>(cfg, BGPParams(cfg), "rcn");
}

inline std::unique_ptr<Simulator::ISimulation> MakeSimulation(SimulationConfig cfg)
{
	if (cfg.protocol == RPBOB)
	{
		switch (cfg.algebra)
		{
		case LENGTHS:
		case HOPS:
			return RPBOBSimulation<Sequenced<L>>(cfg);
		}
	}
	else if (cfg.protocol == RCN)
	{
		switch (cfg.algebra)
		{
		case LENGTHS:
		case HOPS:
			return RCNSimulation<L>(cfg);
		}
	}
	else if (cfg.protocol == BGP)
	{
		switch (cfg.algebra)
		{
		case LENGTHS:
		case HOPS:
			return BGPSimulation<L>(cfg);
		}
	}
	throw std::runtime_error("unknown protocol");
}
} // namespace config

#endif

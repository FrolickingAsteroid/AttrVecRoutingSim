#ifndef ARGPARSING_HPP
#define ARGPARSING_HPP

#include <map>
#include <memory>
#include <string>

#include "core/config.hpp"
#include "routing-algebras/algebras.hpp"
#include "simulator/instances/becing-simulator.hpp"
#include "simulator/instances/bgp-simulator.hpp"
#include "simulator/instances/rcn-simulator.hpp"
#include "simulator/instances/rpbob-simulator.hpp"
#include "simulator/instances/window-simulator.hpp"

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

int ParseDest(const std::string &s);

// ---------------------------------------------------------------------------- *
//
// Parameter map builders
//
// ---------------------------------------------------------------------------- *

inline std::string ScenarioName(Type m)
{
	switch (m)
	{
	case Normal:
		return "normal";
	case LinkFailure:
		return "linkFailure";
	case Cutset:
		return "cutset";
	default:
		throw std::runtime_error("unknown scenario");
	}
}

inline std::map<std::string, std::string> RPBOBParams(const SimulationConfig &cfg)
{
	return {
	    {"notif",   std::to_string(cfg.notif)                                       },
	    {"append",  std::to_string(cfg.append)                                      },
	    {"A_",      cfg.algebra						     },
	    {"N_",      cfg.N.has_value() ? std::to_string(cfg.N.value()) : "adaptative"},
	    {"period_", formatParam(cfg.period.value())                                 },
	    {"S_",      ScenarioName(cfg.scenario)                                      },
	};
}

inline std::map<std::string, std::string> BeaconingParams(const SimulationConfig &cfg)
{
	return {
	    {"append",  std::to_string(cfg.append)     },
            {"A_",      cfg.algebra                    },
	    {"dprop_",  formatParam(cfg.dprop.value()) },
            {"period_", formatParam(cfg.period.value())},
	    {"S_",      ScenarioName(cfg.scenario)     },
	};
}

inline std::map<std::string, std::string> BGPParams(const SimulationConfig &cfg)
{
	return {
	    {"append", std::to_string(cfg.append)},
	    {"A_",     cfg.algebra               },
	    {"S_",     ScenarioName(cfg.scenario)},
	};
}

inline std::map<std::string, std::string> WindParams(const SimulationConfig &cfg)
{
	return {
	    {"append",  std::to_string(cfg.append)     },
	    {"A_",      cfg.algebra                    },
	    {"period_", formatParam(cfg.period.value())},
	    {"S_",      ScenarioName(cfg.scenario)     },
	};
}

// ---------------------------------------------------------------------------- *
//
// Simulator Factory
//
// ---------------------------------------------------------------------------- *

using SimFactory = std::function<std::unique_ptr<Simulator::ISimulation>(SimulationConfig &)>;

template <class RA>
std::unique_ptr<Simulator::ISimulation> RPBOBSimulation(SimulationConfig &cfg)
{
	return std::make_unique<Simulator::RpBOBSimulation<RA>>(cfg, RPBOBParams(cfg), "rpbob");
}

template <class RA>
std::unique_ptr<Simulator::ISimulation> BeaconingSimulation(SimulationConfig &cfg)
{
	return std::make_unique<Simulator::BCNSimulation<RA>>(cfg, BeaconingParams(cfg), "beaconing");
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

template <class RA>
std::unique_ptr<Simulator::ISimulation> ComputeWindows(SimulationConfig &cfg)
{
	return std::make_unique<Simulator::WindowSimulation<RA>>(cfg, WindParams(cfg), "window");
}

struct AlgebraFactories
{
	SimFactory rcn;
	SimFactory bgp;
	SimFactory beaconing;
	SimFactory rpbob;
	SimFactory n;
};

template <class RA>
AlgebraFactories MakeFactories()
{
	return AlgebraFactories{&RCNSimulation<RA>, &BGPSimulation<RA>, &BeaconingSimulation<RA>,
				&RPBOBSimulation<RoutingAlgebras::Sequenced<RA>>,
				&ComputeWindows<RoutingAlgebras::Sequenced<RA>>};
}

inline const std::unordered_map<std::string, AlgebraFactories> &Algebras()
{
	using namespace RoutingAlgebras;
	static const std::unordered_map<std::string, AlgebraFactories> table = {
	    {"lex-hops-suffix",               MakeFactories<LEX_LENGTH_HOPS_SUFFIX>()       }, // used with delayHops
	    {"lex-length-hops-suffix",        MakeFactories<LEX_LENGTH_HOPS_SUFFIX>()       },
	    {"lex-length-width-suffix",       MakeFactories<LEX_LENGTH_WIDTH_SUFFIX>()      },
	    {"prod-width-length-hops-suffix", MakeFactories<PROD_WIDTH_LENGTH_HOPS_SUFFIX>()},
	    {"lex-length-explicit",           MakeFactories<LEX_LENGTH_EXPLICIT>()          },
	    {"lex-hops-explicit",             MakeFactories<LEX_LENGTH_HOPS_SUFFIX>()       }, // used with delayHops
	};
	return table;
}

inline std::unique_ptr<Simulator::ISimulation> MakeSimulation(SimulationConfig cfg)
{
	auto it = Algebras().find(cfg.algebra);
	if (it == Algebras().end())
	{
		throw std::runtime_error("unknown algebra: " + cfg.algebra);
	}

	const auto &[rcn, bgp, beaconing, rpbob, n] = it->second;

	switch (cfg.protocol)
	{
	case RPBOB:
		return rpbob(cfg);
	case RCN:
		return rcn(cfg);
	case BGP:
		return bgp(cfg);
	case Beaconing:
		return beaconing(cfg);
	case N:
		return n(cfg);
	default:
		throw std::runtime_error("unknown protocol");
	}
}
} // namespace config

#endif

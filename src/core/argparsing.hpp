#ifndef ARGPARSING_HPP
#define ARGPARSING_HPP

#include "routing-algebras/sequenced-ord.hpp"
#include "routing-algebras/sequenced-ext.hpp"

#include "simulator/simulator.hpp"
#include "simulator/becing-simulator.hpp"
#include "simulator/rpbob-simulator.hpp"
#include "utils/helpers/helpers.hpp"

#include "utils/ui/appearance.hpp"
#include "utils/register/data.hpp"


using namespace RoutingAlgebras;
using namespace utils::helpers;
using namespace utils::ui;
using namespace std;

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
	optional<double> dprop = nullopt;
	optional<double> period = nullopt;
	optional<int> max_beacons = nullopt;
	optional<std::size_t> window_size = nullopt;

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
	if (cfg.protocol == RPBOB) {
		switch (cfg.algebra) {
		case LENGTHS:
		case HOPS:
			return RPBOBSimulation<int, OSL, ESL>(cfg);
		}
	} else if (cfg.protocol == Beaconing) {
		switch (cfg.algebra) {
		case LENGTHS:
		case HOPS:
			return BeaconingSimulation<int, TOL, LE>(cfg);
		}
	}

	throw std::runtime_error("unknown protocol");
}

}
#endif

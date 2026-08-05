#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <optional>

namespace config
{
enum ScenarioType
{
	Normal,
	LinkFailure,
	Withdrawal
};

enum Options
{
	RPBOB,
	Beaconing,
	BGP,
	RCN,
	N
};

enum Metric
{
	LENGTHS,
	HOPS
};

struct SimulationConfig
{
	// required args
	std::string network_file;
	Options     protocol    = Options::N;
	Metric      algebra     = LENGTHS;
	int         destination = -1;

	// append to existing file
	bool append = false;

	// RPBOB: run with notification
	bool notif = false;

	// unsigned int seed = 1307683871;
	unsigned int seed = 0;

	// optional args
	std::optional<double> dprop  = std::nullopt;
	std::optional<double> period = std::nullopt;
	std::optional<int>    max    = std::nullopt;
	std::optional<int>    N      = std::nullopt;

	// Type of simulation being run
	ScenarioType scenario   = Normal;
	int          u          = -1; // used by LinkFailure
	int          v          = -1; // used by LinkFailure
	int          withdrawal = -1; // used by Withdrawal
};
} // namespace config
#endif

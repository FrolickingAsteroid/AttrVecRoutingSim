#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <optional>

namespace config
{
enum Type
{
	Normal,
	LinkFailure,
	Cutset,
};

enum Options
{
	RPBOB,
	Beaconing,
	BGP,
	RCN,
	N
};

struct SimulationConfig
{
	// required args
	std::string network_file;

	Options     protocol    = Options::N;
	std::string algebra     = "lex-length-hops-suffix";
	int         destination = -1;

	// append to existing file
	bool append = false;

	// RPBOB: run with notification
	bool notif = false;

	// simulation seed
	//unsigned int seed;

	unsigned int seed = 2535664635;
	//   unsigned int seed = 2425713638;

	// optional args
	std::optional<double> dprop  = std::nullopt;
	std::optional<double> period = std::nullopt;
	std::optional<int>    max    = std::nullopt;
	std::optional<int>    N      = std::nullopt;

	// Type of simulation being run
	Type scenario   = Normal;
	int  u          = -1; // used by LinkFailure
	int  v          = -1; // used by LinkFailure
	int  withdrawal = -1; // used by Withdrawal
};
} // namespace config
#endif

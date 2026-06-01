
#include <cstdlib>
#include <memory>
#include <string>
#include <stdexcept>
#include <iostream>
#include <unordered_set>
#include <tabulate/tabulate.hpp>

#include "core/argparsing.hpp"

namespace config
{

void PrintUsage(const char *prog)
{
	std::cerr
		<< yellow("\nusage:\n") << "  " << prog
		<< " -f <network.tsv> -R|-G|-B [-a <algebra>]"
		<< " [-t <dest|all>] [-r <stab|set>]\n"
		<< "  " << prog
		<< " -R -f <network.tsv> -i <period> -m <max> [-w <N>]\n"
		<< "  " << prog
		<< " -B -f <network.tsv> -i <period> -m <max> -d <dprop>\n"
		<< "  " << prog
		<< " -N -f <network.tsv> -i <period> [-a <algebra>]\n\n"

		<< yellow("protocols:\n")
		<< "  -R               RPBOB (default)\n"
		<< "  -G               BGP\n"
		<< "  -B               Beaconing\n"
		<< "  -N               compute window size N from specified network file and period\n\n"

		<< yellow("required:\n")
		<< "  -f <file>        path to network .tsv file\n"
		<< "  -i <period>      destination announcement period\n"
		<< "  -m <max>         number of routing computations\n\n"

		<< yellow("optional:\n")
		<< "  -a <algebra>     routing algebra: lengths (default), hops\n"
		<< "  -t <dest>        destination node id, or 'all' (default: all)\n"
		<< "  -r <type>        data to collect: node stabilization time, stab or time series, set (default: stab)\n"
		<< "  -w <N>           window size for RPBOB (default: auto from graph)\n"
		<< "  -d <dprop>       propagation interval for Beaconing\n\n";
}

void PrintSummary(std::chrono::milliseconds elapsed)
{
	tabulate::Table table;
	table.add_row(Row({ "Parameter", "value" }));

	table.add_row(Row(
		{ "elapsed time", std::to_string(elapsed.count()) + " ms" }));
	table[0].format()
		.font_style({ tabulate::FontStyle::bold })
		.font_color(tabulate::Color::yellow);

	std::cout << cyan("\nSimulation summary:\n");
	std::cout << table << "\n";
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

Metric ParseAlgebra(const std::string &s)
{
	if (s == "length") {
		return LENGTHS;
	}
	if (s == "hops") {
		return HOPS;
	}
	throw std::runtime_error("unknown data collection: " + s);
}

SimulationConfig ParseArgs(int argc, char *const *argv)
{
	SimulationConfig cfg;
	int c;
	while ((c = getopt(argc, argv, "f:t:r:m:d:a:w:i:NRGB")) != -1) {
		switch (c) {
		case 'f':
			cfg.network_file = optarg;
			break;
		case 'R':
			cfg.protocol = RPBOB;
			break;
		case 'G':
			cfg.protocol = BGP;
			break;
		case 'B':
			cfg.protocol = Beaconing;
			break;
		case 'a':
			cfg.algebra = ParseAlgebra(optarg);
			break;
		case 't':
			cfg.destination = ParseDest(optarg);
			break;
		case 'r':
			cfg.register_type = ParseRegisterType(optarg);
			break;
		case 'm':
			cfg.max_beacons = std::stoi(optarg);
			break;
		case 'i':
			cfg.period = std::stod(optarg);
			break;
		case 'd':
			cfg.dprop = std::stod(optarg);
			break;
		case 'w':
			cfg.window_size = std::stoul(optarg);
			break;
		case 'N':
			cfg.protocol = N;
			break;
		case '?':
		case 'h':
		default:
			PrintUsage(argv[0]);
			std::exit(0);
		}
	}

	if (cfg.network_file.empty()) {
		throw std::runtime_error("must specify a network file");
	}

	if (cfg.protocol == RPBOB) {
		if (cfg.period == nullopt || cfg.max_beacons == nullopt ||
		    cfg.window_size == nullopt) {
			throw std::runtime_error(
				"must specify period, window size, and number of routing computations");
		}
	} else if (cfg.protocol == Beaconing) {
		if (cfg.period == nullopt || cfg.max_beacons == nullopt ||
		    cfg.dprop == nullopt) {
			throw std::runtime_error(
				"must specify propagation delay, period and number of routing computations");
		}
	} else if (cfg.protocol == N) {
		if (cfg.period == nullopt) {
			throw std::runtime_error(
				"must period of routing computations");
		}
		switch (cfg.algebra) {
		case LENGTHS:
		case HOPS:
			if (cfg.destination != -1) {
				Compute_N<int, TOL, LE>(cfg.network_file,
							cfg.period.value(),
							cfg.destination);
			} else {
				Compute_N<int, TOL, LE>(cfg.network_file,
							cfg.period.value());
			}
		}
		std::exit(0);
	}

	return cfg;
}
}
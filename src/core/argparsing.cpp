#include "core/argparsing.hpp"

#include <getopt.h>
#include <tabulate/tabulate.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "utils/helpers/helpers.hpp"
#include "utils/ui/logger.hpp"

using namespace RoutingAlgebras;
using namespace utils::helpers;
using namespace utils::ui;
using namespace std;

namespace config
{
void PrintUsage(const char *prog)
{
	std::cerr << yellow("\nusage:\n") << "  " << prog << " -f <network.tsv> -<protocol>"
		  << " [options...]\n\n"

		  << "  " << prog << " -f <network.tsv> -R -i <period> [-m <max>] [-w <N>]\n"
		  << "  " << prog << " -f <network.tsv> -B -i <period> -d <dprop> [-m <max>]\n"
		  << "  " << prog << " -f <network.tsv> -G\n"
		  << "  " << prog << " -f <network.tsv> -C\n\n"

		  << yellow("protocols:\n")
		  << "  -R, --rpbob[=notif]   RPBOB, optionally with link failure notification\n"
		  << "  -G, --bgp             BGP\n"
		  << "  -C, --rcn             BGP with RCN\n"
		  << "  -B, --beaconing       Beaconing\n"
		  << "  -N, --wind            compute window size N from network file and a arbitrary period\n\n"

		  << yellow("required:\n") << "  -f, --file <file>       path to network .tsv file\n"
		  << "  -i, --period <period>   destination announcement period (beaconing and rpbob)\n"
		  << "  -d, --dprop <dprop>     propagation event interval (beaconing) \n"

		  << yellow("optional:\n")
		  << "  -m, --max <max>         number of routing computation events to schedule (beaconing and "
		     "rpbob) \n"
		  << "  -a, --algebra <algebra> routing algebra: shortest paths (default), hops\n"
		  << "  -t, --dest <dest>       destination node id, or 'all' (default: all)\n"
		  << "  -w, --window <N>        window size for RPBOB (default: deduced from dataset)\n"

		  << "  -F, --failure [u v]     simulate a link failure, at 'uv' (default: all)\n"
		  << "  -W, --withdraw <n>      simulate a node withdrawal at 'n' (default: all)\n"
		  << "  -A, --append            append results to output file instead of overwriting\n\n"

		  << yellow("log level:\n") << "  -I, --info       print basic information (default)\n"
		  << "  -V, --verbose    print additional simulation information\n"
		  << "  -S, --silent     suppress printing\n\n"

		  << "  -h, --help       show this message\n";
}

void PrintSummary(std::chrono::milliseconds elapsed, const utils::stats::SummaryStats &stats)
{
	using namespace tabulate;
	using namespace utils::stats;

	auto fmt = [](const double v) {
		std::ostringstream ss;
		ss << std::fixed << std::setprecision(2) << v;
		return ss.str();
	};
	auto range = [&](const MsgStats &s) {
		return fmt(s.avg) + "  (min: " + fmt(s.min) + " / max: " + fmt(s.max) + ")";
	};

	Table table;
	table.add_row({"Parameter", "Avg  (min / max)"});
	table.add_row({"Elapsed time", std::to_string(elapsed.count()) + " ms"});
	table.add_row({"Total nodes processed", std::to_string(stats.total_nodes)});
	table.add_row({"Termination time (ms)", range(stats.term)});
	table.add_row({"Attribute changes / node", range(stats.changes)});

	table[0].format().font_style({FontStyle::bold}).font_color(Color::yellow);
	for (std::size_t i = 1; i < table.size(); ++i)
		table[i][0].format().font_color(Color::cyan);

	LOG_INFO << purple("\nSimulation summary:\n");
	LOG_INFO << table << "\n\n";
}

int ParseDest(const std::string &s)
{
	if (s == "all")
		return -1;
	return std::stoi(s);
}

Metric ParseAlgebra(const std::string &s)
{
	if (s == "length")
		return LENGTHS;
	if (s == "hops")
		return HOPS;
	throw std::runtime_error("unknown algebra: " + s);
}

SimulationConfig ParseArgs(int argc, char *const *argv)
{

	if (argc == 1)
	{
		PrintUsage(argv[0]);
		std::exit(0);
	}

	SimulationConfig cfg;
	int              c;

	struct option long_options[] = {
	    {"file",     required_argument, nullptr,              'f'      },
	    {"dest",     required_argument, nullptr,              't'      },
	    {"max",      required_argument, nullptr,              'm'      },
	    {"dprop",    required_argument, nullptr,              'd'      },
	    {"algebra",  required_argument, nullptr,              'a'      },
	    {"window",   required_argument, nullptr,              'w'      },
	    {"period",   required_argument, nullptr,              'i'      },
	    {"failure",  optional_argument, nullptr,              'F'      },
	    {"withdraw", required_argument, nullptr,              'W'      },
	    {"rpbob",    optional_argument, nullptr,              'R'      },
	    {"bgp",      no_argument,       nullptr,              'G'      },
	    {"rcn",      no_argument,       (int *)&cfg.protocol, RCN      },
	    {"beacon",   no_argument,       (int *)&cfg.protocol, Beaconing},
	    {"W",        no_argument,       (int *)&cfg.protocol, N        },
	    {"verbose",  no_argument,       nullptr,              'V'      },
	    {"silent",   no_argument,       nullptr,              'S'      },
	    {"info",     no_argument,       nullptr,              'I'      },
	    {"append",   no_argument,       (int *)&cfg.append,   1        },
	    {"help",     no_argument,       nullptr,              'h'      },
	    {nullptr,    0,                 nullptr,              0        },
	};

	while ((c = getopt_long(argc, argv, "f:t:m:d:a:w:i:F::W:NR::GCBVSIA", long_options, nullptr)) != -1)
	{
		switch (c)
		{
		case 'f':
			cfg.network_file = optarg;
			break;
		case 'R':
			cfg.protocol = RPBOB;
			if (optind < argc && *argv[optind] != '-')
			{
				try
				{
					if (std::string(argv[optind]) == "notif")
					{
						cfg.notif = true;
					}
				}
				catch (...)
				{
					throw std::runtime_error("Unknown Option");
				}
			}
			break;
		case 'G':
			cfg.protocol = BGP;
			break;
		case 'a':
			cfg.algebra = ParseAlgebra(optarg);
			break;
		case 't':
			cfg.destination = ParseDest(optarg);
			break;
		case 'm':
			cfg.max = std::stoi(optarg);
			break;
		case 'i':
			cfg.period = std::stod(optarg);
			break;
		case 'd':
			cfg.dprop = std::stod(optarg);
			break;
		case 'w':
			cfg.N = std::stoul(optarg);
			break;
		case 'F': {
			cfg.scenario = LinkFailure;
			if (optind < argc && *argv[optind] != '-')
			{
				try
				{
					cfg.u = std::stoi(argv[optind]);
					++optind;
					cfg.v = std::stoi(argv[optind]);
				}
				catch (...)
				{
					throw std::runtime_error(
					    "Either two nodes must be specified or no node at all");
				}
			}
			break;
		}
		case 'W': {
			cfg.scenario   = Withdrawal;
			cfg.withdrawal = std::stoi(optarg);
			break;
		}
		case 'V':
			setVerbose();
			break;
		case 'S':
			setSilent();
			break;
		case 'I':
			setInfo();
			break;
		case 0:
			break;
		case '?':
		case 'h':
		default:
			PrintUsage(argv[0]);
			std::exit(0);
		}
	}
	if (cfg.network_file.empty())
	{
		throw std::runtime_error("must specify a network file");
	}
	if (cfg.network_file.empty())
	{
		throw std::runtime_error("must specify a network file");
	}
	if (cfg.protocol == RPBOB)
	{
		if (!cfg.period)
			throw std::runtime_error("must specify period");
	}
	if (cfg.protocol == Beaconing)
	{
		if (!cfg.period)
			throw std::runtime_error("must specify period");
		if (!cfg.dprop)
			throw std::runtime_error("must specify propagation delay");
	}
	return cfg;
}
} // namespace config
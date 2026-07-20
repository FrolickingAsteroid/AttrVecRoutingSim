#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tabulate/tabulate.hpp>

#include "core/argparsing.hpp"
#include "utils/helpers/helpers.hpp"
#include "utils/ui/logger.hpp"

using namespace RoutingAlgebras;
using namespace utils::helpers;
using namespace utils::ui;
using namespace std;

namespace config {
    void PrintUsage(const char *prog) {
        std::cerr << yellow("\nusage:\n") << "  " << prog
                << " -f <network.tsv> -R|-G|-B [-a <algebra>]"
                << " [-t <dest|all>]\n"
                << "  " << prog << " -R -f <network.tsv> -i <period> -m <max> [-w <N>]\n"
                << "  " << prog << " -B -f <network.tsv> -i <period> -m <max> -d <dprop>\n"
                << "  " << prog << " -N -f <network.tsv> -i <period> [-a <algebra>]\n\n"

                << yellow("protocols:\n") << "  -R               RPBOB (default)\n"
                << "  -G               BGP\n"
                << "  -B               Beaconing\n"
                << "  -N               compute window size N from network file and period\n\n"

                << yellow("required:\n") << "  -f <file>        path to network .tsv file\n"
                << "  -i <period>      destination announcement period\n"
                << "  -m <max>         number of routing computations\n\n"

                << yellow("optional:\n")
                << "  -a <algebra>     routing algebra: lengths (default), hops\n"
                << "  -t <dest>        destination node id, or 'all' (default: all)\n"
                << "  -w <N>           window size for RPBOB (default: auto from graph)\n"
                << "  -d <dprop>       propagation interval for Beaconing\n\n"

                << yellow("log level:\n")
                << "  -I               print basic information (default)\n"
                << "  -V               print additional protocol information\n"
                << "  -S               suppress printing\n";
    }

    void PrintSummary(std::chrono::milliseconds elapsed, const utils::stats::SummaryStats &stats) {
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

        const bool has_failures = !stats.msgs_per_link_failure.empty();

        Table table;
        table.add_row({"Parameter", "Avg  (min / max)"});
        table.add_row({"Elapsed time", std::to_string(elapsed.count()) + " ms"});
        table.add_row({"Total nodes processed", std::to_string(stats.total_nodes)});
        table.add_row({"Termination time (ms)", range(stats.term)});
        table.add_row({"Attribute changes / node", range(stats.changes)});
        table.add_row({"Messages / node", range(stats.nodes)});
        table.add_row({
            has_failures ? "Messages / failure" : "Messages / destination",
            range(stats.msg)
        });

        table[0].format().font_style({FontStyle::bold}).font_color(Color::yellow);
        for (std::size_t i = 1; i < table.size(); ++i)
            table[i][0].format().font_color(Color::cyan);

        LOG_INFO << purple("\nSimulation summary:\n");
        LOG_INFO << table << "\n\n";
    }

    int ParseDest(const std::string &s) {
        if (s == "all")
            return -1;
        return std::stoi(s);
    }

    Metric ParseAlgebra(const std::string &s) {
        if (s == "length")
            return LENGTHS;
        if (s == "hops")
            return HOPS;
        throw std::runtime_error("unknown algebra: " + s);
    }

    SimulationConfig ParseArgs(int argc, char *const *argv) {
        SimulationConfig cfg;
        int c;

        while ((c = getopt(argc, argv, "f:t:m:d:a:w:i:F::W:NR::GCBVSIA")) != -1) {
            switch (c) {
                case 'f':
                    cfg.network_file = optarg;
                    break;
                case 'R':
                    cfg.protocol = RPBOB;
                    if (optind < argc && *argv[optind] != '-') {
                        try {
                            if (std::string(argv[optind]) == "notif") {
                                cfg.notif = true;
                            }
                        } catch (...) {
                            throw std::runtime_error(
                                "Unknown Option");
                        }
                    }
                    break;
                case 'G':
                    cfg.protocol = BGP;
                    break;
                case 'C':
                    cfg.protocol = RCN;
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
                case 'N':
                    cfg.protocol = N;
                    break;
                case 'F': {
                    cfg.scenario = LinkFailure;
                    if (optind < argc && *argv[optind] != '-') {
                        try {
                            cfg.u = std::stoi(argv[optind]);
                            ++optind;
                            cfg.v = std::stoi(argv[optind]);
                        } catch (...) {
                            throw std::runtime_error(
                                "Either two nodes must be specified or no node at all");
                        }
                    }
                    break;
                }

                case 'W': {
                    cfg.scenario = Withdrawal;
                    cfg.withdrawal = std::stoi(optarg);
                    break;
                }

                case 'V':
                    setVerbose();
                    break;
                case 'A':
                    cfg.append = true;
                    break;
                case 'S':
                    setSilent();
                    break;
                case 'I':
                    setInfo();
                    break;
                case '?':
                case 'h':
                default:
                    PrintUsage(argv[0]);
                    std::exit(0);
            }
        }

        if (cfg.network_file.empty())
            throw std::runtime_error("must specify a network file");

        if (cfg.protocol == RPBOB) {
            if (!cfg.period || !cfg.max)
                throw std::runtime_error(
                    "must specify period, and number of routing computations");
        } else if (cfg.protocol == Beaconing) {
            if (!cfg.period || !cfg.max || !cfg.dprop)
                throw std::runtime_error(
                    "must specify propagation delay, period, and number of routing computations");
        } else if (cfg.protocol == N) {
            if (!cfg.period)
                throw std::runtime_error("must specify period");

            switch (cfg.algebra) {
                case LENGTHS:
                case HOPS:
                    Compute_N<int, TOL, LE>(cfg.network_file, cfg.period.value());
                    break;
            }
            std::exit(0);
        }
        return cfg;
    }
}

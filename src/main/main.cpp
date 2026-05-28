#include "main/argparsing.hpp"

#include <chrono>
#include <string>

using namespace std;
using namespace config;

std::string DestName(int dest)
{
	if (dest < 0)
		return "all";

	return std::to_string(dest);
}

std::string ParamName(const SimulationConfig &cfg)
{
	if (cfg.protocol == "seq-length")
		return "N";

	if (cfg.protocol == "beacon-length")
		return "dprop";

	return "-";
}

std::string ParamValue(const SimulationConfig &cfg)
{
	std::ostringstream os;

	if (cfg.protocol == "seq-length" && cfg.window_size)
		os << *cfg.window_size;
	else if (cfg.protocol == "beacon-length" && cfg.dprop)
		os << *cfg.dprop;
	else
		os << "-";

	return os.str();
}

// void PrintConfig(const SimulationConfig &cfg)
// {
// 	tabulate::Table table;
//
// 	table.add_row(Row({ "Parameter", "Value" }));
// 	table.add_row(Row({ "network file", cfg.network_file }));
// 	table.add_row(Row({ "protocol", cfg.protocol }));
// 	table.add_row(Row({ "destination", DestName(cfg.destination) }));
// 	table.add_row(Row({ "max beacons", std::to_string(cfg.max_beacons) }));
// 	table.add_row(Row({ "period", std::to_string(cfg.period) }));
// 	table.add_row(Row({ ParamName(cfg), ParamValue(cfg) }));
//
// 	table[0].format()
// 		.font_style({ tabulate::FontStyle::bold })
// 		.font_color(tabulate::Color::yellow);
//
// 	std::cout << "\nSimulation parameters\n";
// 	std::cout << table << "\n";
// }

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

int main(int argc, const char **argv)
{
	try {
		const auto cfg = ParseArgs(argc, argv);
		auto sim = MakeSimulation(cfg);

		sim->setup();

		const auto start = chrono::steady_clock::now();
		sim->run();
		const auto end = chrono::steady_clock::now();

		const auto elapsed =
			chrono::duration_cast<chrono::milliseconds>(end -
								    start);

		PrintSummary(elapsed);

		return 0;
	} catch (const std::exception &err) {
		std::cerr << "error: " << err.what() << '\n';
		return 1;
	}
}
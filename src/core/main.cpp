#include "core/argparsing.hpp"
#include "utils/stats/data.hpp"

#include <chrono>

using namespace std;
using namespace config;

int main(int argc, char *const *argv)
{
	try {
		const auto cfg = ParseArgs(argc, argv);
		auto	   sim = MakeSimulation(cfg);

		sim->setup();

		const auto start = chrono::steady_clock::now();
		sim->run();
		const auto end = chrono::steady_clock::now();

		auto stats = sim->postProcess();

		const auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);

		PrintSummary(elapsed,stats);

		return 0;
	} catch (const std::exception &err) {
		std::cerr << "error: " << err.what() << '\n';
		return 1;
	}
}
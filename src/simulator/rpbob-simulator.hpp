#ifndef RPBOB_SIMULATION_HPP
#define RPBOB_SIMULATION_HPP

#include "utils/ui/appearance.hpp"
#include "protocol/rpbob/rpbob.hpp"
#include "simulator/simulator.hpp"

#include <cstddef>
#include <numeric>
#include <string>

using namespace Protocol::Rpbob;
using namespace Graphs;

namespace Simulator
{

template <class RA> class RpBOBSimulation : public Simulation<Path<typename RA::Attr>> {
    public:
	using Base = Simulation<Path<typename RA::Attr>>;
	using Base::registrar;

	RpBOBSimulation(const std::string &filename, std::size_t N_, int max_, int dest_,
			double period_, utils::RegisterType log_t_)
		: Base(filename, max_, dest_, period_, log_t_, "rpbob", "N",
		       std::to_string(N_))
		, graph(Graph<typename RA::BaseAttr>::readGraph(filename))
		, period(period_)
		, dest(dest_)
		, max(max_)
		, N(N_)
	{
	}

	void setup() override
	{
		if (dest < 0) {
			targets.resize(graph.V);
			std::iota(targets.begin(), targets.end(), 0);
		} else {
			targets = { dest };
		}
	}

	void run() override
	{
		for (int t : targets) {
			RPBOB<RA> proto(graph, N);
			proto.DestinationAnnouncement(t, 0, period, max);
			proto.Run();
			registrar.RegisterDestination(t, proto.data);
		}
	}

    private:
	Graph<typename RA::BaseAttr> graph;
	double period;
	int dest, max;
	std::size_t N;
	std::vector<int> targets;
};

}

#endif
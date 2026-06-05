#ifndef BECING_SIMULATION_HPP
#define BECING_SIMULATION_HPP

#include <numeric>
#include <string>

#include "protocol/beaconing/beaconing.hpp"
#include "simulator/simulator.hpp"

#include "utils/ui/appearance.hpp"

using namespace Protocol::Beaconing;
using namespace Graphs;

namespace Simulator
{

template <class RA> class BECINGSimulation : public Simulation<Beacon<typename RA::BaseAttr>> {
    public:
	using Base = Simulation<Beacon<typename RA::BaseAttr>>;
	using Base::registrar;

	BECINGSimulation(const std::string &filename, int max_, int dest_, double d_prop_,
			 double period_, utils::RegisterType log_t_)
		: Base(filename, max_, dest_, period_, log_t_, "beaconing", "dprop",
		       utils::ui::formatParam(d_prop_))
		, graph(Graph<typename RA::BaseAttr>::readGraph(filename))
		, d_prop(d_prop_)
		, period(period_)
		, dest(dest_)
		, max(max_)
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
			BECING<RA> proto(graph);
			proto.DestinationAnnouncement(t, 0, period, max);

			// periodic propagation on every node,
			// independent of destination announcement
			for (int u = 0; u < graph.V; ++u) {
				proto.SchedulePropagation(u, d_prop, max);
			}

			proto.Run();
			registrar.RegisterDestination(t, proto.data);
		}
	}

    private:
	Graph<typename RA::BaseAttr> graph;
	double d_prop;
	double period;
	int dest, max;
	std::vector<int> targets;
};

} // namespace Simulator

#endif
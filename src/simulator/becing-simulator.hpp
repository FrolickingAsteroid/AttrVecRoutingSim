#ifndef BECING_SIMULATION_HPP
#define BECING_SIMULATION_HPP

#include <numeric>
#include <string>

#include "protocol/beaconing/beaconing.hpp"
#include "simulator/simulator.hpp"

#include "utils/ui/monitor.hpp"

using namespace Protocol::Beaconing;

namespace Simulator
{

template <class A, class Ord, class Ext>
class BECINGSimulation : public Simulation<A, Beacon<A> > {
    public:
	using Base = Simulation<A, Beacon<A> >;
	using Base::ReadGraph;
	using Base::registrar;

	BECINGSimulation(const std::string &filename, int max_, int dest_,
			 double d_prop_, double period_,
			 utils::RegisterType log_t_)
		: Base(filename, max_, dest_, period_, log_t_, "beaconing")
		, graph(ReadGraph(filename))
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
			BECING<A, Ord, Ext> proto(graph);
			proto.DestinationAnnouncement(t, 0, period, max);

			// periodic propagation on every node,
			// independent destination announcement
			for (int u = 0; u < graph.V; ++u) {
				proto.SchedulePropagation(u, d_prop, max);
			}

			proto.Run();
			registrar.RegisterDestination(t, proto.data);
		}
	}

    private:
	Graphs::Graph<A> graph;
	double d_prop;
	double period;
	int dest, max;
	std::vector<int> targets;
};

} // namespace Simulator

#endif
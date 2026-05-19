#ifndef RPBOB_SIMULATION_HPP
#define RPBOB_SIMULATION_HPP

#include "protocol/rpbob/rpbob.hpp"
#include "simulator/simulator.hpp"
#include <cstddef>
#include <numeric>
#include <string>

using namespace Protocol::Rpbob;

namespace Simulator
{

template <class A, class Ord, class Ext>
class RpBOBSimulation : public Simulation<A, Path<A> > {
    public:
	using Base = Simulation<A, Path<A> >;
	using Base::ReadGraph;
	using Base::registrar;

	RpBOBSimulation(const std::string &filename, std::size_t N_, int max_,
			int dest_, double period_, utils::RegisterType log_t_)
		: Base(filename, max_, dest_, period_, log_t_, "rpbob")
		, graph(ReadGraph(filename))
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
			RPBOB<A, Ord, Ext> proto(graph, N);
			proto.DestinationAnnouncement(t, 0, period, max);
			proto.Run();
			registrar.RegisterDestination(t, proto.data);
		}
	}

    private:
	Graphs::Graph<A> graph;
	double period;
	int dest, max;
	std::size_t N;
	std::vector<int> targets;
};

} // namespace Simulator

#endif
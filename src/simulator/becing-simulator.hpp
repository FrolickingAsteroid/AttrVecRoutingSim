#ifndef BECING_SIMULATION_HPP
#define BECING_SIMULATION_HPP

#include <numeric>
#include <string>

#include "simulator/simulator.hpp"
#include "utils/ui/appearance.hpp"
#include "utils/stats/csv-writer.hpp"
#include "protocol/beaconing/beaconing.hpp"

using namespace Protocol::Beaconing;
using namespace Graphs;

namespace Simulator
{

template <class RA> class BECINGSimulation : public ISimulation {
    public:
	BECINGSimulation(const std::string			  &filename,
			 const std::map<std::string, std::string> &map,
			 int					   max_,
			 int					   dest_,
			 double					   d_prop_,
			 double					   period_)
		: graph(Graph<typename RA::BaseAttr>::readGraph(filename))
		, recorder(filename, "beaconing", map)
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
			recorder.recordDest(t, proto.data);
		}
	}

    private:
	Graph<typename RA::BaseAttr>			      graph;
	utils::stats::Recorder<Beacon<typename RA::BaseAttr>> recorder;
	double						      d_prop;
	double						      period;
	int						      dest, max;
	std::vector<int>				      targets;
};

} // namespace Simulator

#endif
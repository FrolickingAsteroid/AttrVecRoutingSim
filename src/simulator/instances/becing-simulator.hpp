#ifndef BECING_SIMULATION_HPP
#define BECING_SIMULATION_HPP

#include <numeric>
#include <string>

#include "core/config.hpp"
#include "protocol/beaconing/beaconing.hpp"
#include "simulator/simulator.hpp"
#include "utils/stats/csv-writer.hpp"

using namespace Protocol::Beaconing;
using namespace Graphs;

namespace Simulator
{

template <class RA> class BECINGSimulation : public ISimulation
{
    public:
	BECINGSimulation(config::SimulationConfig &cfg_, const std::map<std::string, std::string> &map)
	    : cfg(cfg_)
	    , graph(Graph<typename RA::BaseAttr>::readGraph(cfg.network_file))
	    , recorder(cfg.network_file, "rpbob", map)
	{
	}

	void setup() override
	{
		if (cfg.destination < 0)
		{
			targets.resize(graph.V);
			std::iota(targets.begin(), targets.end(), 0);
		}
		else
		{
			targets = {cfg.destination};
		}
	}

	void run() override
	{
		for (int t : targets)
		{
			BECING<RA> proto(graph);
			proto.DestinationAnnouncement(t, 0, *cfg.period, *cfg.max);

			// periodic propagation on every node,
			// independent of destination announcement
			for (int u = 0; u < graph.V; ++u)
			{
				proto.SchedulePropagation(u, *cfg.dprop, *cfg.max);
			}

			proto.Run();
			recorder.recordDest(t, proto.data);
		}
	}

	utils::stats::SummaryStats postProcess() override
	{
		return recorder.Summary(graph.V);
	}

    private:
	config::SimulationConfig                             &cfg;
	Graph<typename RA::BaseAttr>                          graph;
	utils::stats::Recorder<Beacon<typename RA::BaseAttr>> recorder;

	std::vector<int> targets;
	double           convergence_time = 0;
};

} // namespace Simulator

#endif
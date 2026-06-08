#ifndef RPBOB_SIMULATION_HPP
#define RPBOB_SIMULATION_HPP

#include <cstddef>
#include <numeric>
#include <string>

#include "core/config.hpp"

#include "simulator/simulator.hpp"
#include "protocol/rpbob/rpbob.hpp"

#include "utils/ui/logger.hpp"
#include "utils/stats/csv-writer.hpp"
#include "utils/stats/stats-summary.hpp"

using namespace Protocol::Rpbob;
using namespace Graphs;

namespace Simulator
{

template <class RA> class RpBOBSimulation : public ISimulation {
    public:
	RpBOBSimulation(config::SimulationConfig		 &cfg_,
			const std::map<std::string, std::string> &map)
		: cfg(cfg_)
		, graph(Graph<typename RA::BaseAttr>::readGraph(cfg.network_file))
		, recorder(cfg.network_file, "rpbob", map)
	{
	}

	void setup() override
	{
		if (cfg.destination < 0) {
			targets.resize(graph.V);
			std::iota(targets.begin(), targets.end(), 0);
		} else {
			targets = { cfg.destination };
		}

		if (cfg.scenario == config::Normal) {
			return;
		}

		double max_ct = 0;
		for (int t : targets) {
			RPBOB<RA> dry(graph, cfg.N.value());
			dry.DestinationAnnouncement(t, 0, 0, 1);
			dry.Run();
			double ct = *std::max_element(dry.data.lastChangeTime.begin(),
						      dry.data.lastChangeTime.end());

			max_ct = std::max(max_ct, ct);
		}
		convergence_time = max_ct;

		LOG_DEBUG << "\nLargest convergence time " << convergence_time << "\n";
	}

	void run() override
	{
		for (int t : targets) {
			RPBOB<RA> proto(graph, *cfg.N);
			proto.data.setOffset(convergence_time);

			if (cfg.scenario == config::ScenarioType::LinkFailure)
				proto.cal.schedule(convergence_time,
						   [&]() { proto.LinkFailure(cfg.u, cfg.v); });

			proto.DestinationAnnouncement(t, 0, *cfg.period, *cfg.max);

			proto.Run();
			recorder.recordDest(t, proto.data);
		}
	}

	utils::stats::SummaryStats postProcess() override
	{
		return utils::stats::Summary(recorder.getRecords());
	}

    private:
	config::SimulationConfig		       &cfg;
	Graph<typename RA::BaseAttr>			graph;
	utils::stats::Recorder<Path<typename RA::Attr>> recorder;

	std::vector<int> targets;
	double		 convergence_time = 0;
};

}

#endif
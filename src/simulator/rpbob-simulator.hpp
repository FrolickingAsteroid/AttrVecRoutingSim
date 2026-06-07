#ifndef RPBOB_SIMULATION_HPP
#define RPBOB_SIMULATION_HPP

#include <cstddef>
#include <numeric>
#include <string>

#include "utils/ui/appearance.hpp"
#include "protocol/rpbob/rpbob.hpp"
#include "simulator/simulator.hpp"
#include "utils/stats/csv-writer.hpp"
#include "utils/stats/stats-summary.hpp"

using namespace Protocol::Rpbob;
using namespace Graphs;

namespace Simulator
{

template <class RA> class RpBOBSimulation : public ISimulation {
    public:
	RpBOBSimulation(const std::string			 &filename,
			const std::map<std::string, std::string> &map,
			std::size_t				  N_,
			int					  max_,
			int					  dest_,
			double					  period_)
		: graph(Graph<typename RA::BaseAttr>::readGraph(filename))
		, recorder(filename, "rpbob", map)
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
			recorder.recordDest(t, proto.data);
		}
	}

	utils::stats::SummaryStats postProcess() override
	{
		return utils::stats::Summary(recorder.getRecords());
	}

    private:
	Graph<typename RA::BaseAttr>			graph;
	utils::stats::Recorder<Path<typename RA::Attr>> recorder;
	double						period;
	int						dest;
	int						max;
	std::size_t					N;
	std::vector<int>				targets;
};

}

#endif
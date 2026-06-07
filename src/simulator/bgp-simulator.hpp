#ifndef BGP_SIMULATION_HPP
#define BGP_SIMULATION_HPP

#include <cstddef>
#include <numeric>
#include <string>

#include "protocol/bgp/bgp.hpp"
#include "simulator/simulator.hpp"
#include "utils/stats/csv-writer.hpp"
#include "utils/stats/stats-summary.hpp"
#include "protocol/bgp/state/bgp-attribute.hpp"

using namespace Protocol::BGP;
using namespace Graphs;

namespace Simulator
{

template <class RA> class BGPSimulation : public ISimulation {
    public:
	BGPSimulation(const std::string			       &filename,
		      const std::map<std::string, std::string> &map,
		      int					dest_)
		: graph(Graph<typename RA::BaseAttr>::readGraph(filename))
		, recorder(filename, "bgp", map)
		, dest(dest_)
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
			BGP<RA> proto(graph);
			proto.DestinationAnnouncement(t);
			proto.Run();
			recorder.recordDest(t, proto.data);
		}
	}

    private:
	Graph<typename RA::BaseAttr>						      graph;
	utils::stats::Recorder<BGPAttribute<typename RA::BaseAttr, typename RA::Ext>> recorder;
	int									      dest;
	std::vector<int>							      targets;
};

}

#endif
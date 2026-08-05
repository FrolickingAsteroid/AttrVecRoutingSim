#ifndef SIMULATOR_COMMON_HPP
#define SIMULATOR_COMMON_HPP

#include <numeric>

#include "core/config.hpp"
#include "graph/graph.hpp"
#include "simulator/simulator.hpp"

namespace Simulator
{
inline std::vector<int> buildTargets(const config::SimulationConfig &cfg, const std::size_t V)
{
	if (cfg.destination >= 0)
	{
		return {cfg.destination};
	}

	std::vector<int> t(V);
	std::iota(t.begin(), t.end(), 0);
	return t;
}

template <class Attr>
std::vector<std::pair<int, int>> buildBidLinks(const config::SimulationConfig &cfg, Graph<Attr> &graph)
{
	if (cfg.u != -1 && cfg.v != -1)
	{
		return {
		    {cfg.u, cfg.v}
                };
	}
	return graph.findBidLinks();
}
template <class Attr, class RecordedT> class SimulationCommon : public ISimulation
{
    public:
	SimulationCommon(config::SimulationConfig &cfg_, const std::map<std::string, std::string> &map, std::string tag)
	    : cfg(cfg_)
	    , graph(Graph<Attr>::readGraph(cfg.network_file))
	    , recorder(cfg.network_file, tag, map)
	{
	}

	void setup() override
	{
		targets   = buildTargets(cfg, graph.V);
		bid_links = buildBidLinks(cfg, graph);
	}

	utils::stats::SummaryStats postProcess() override
	{
		return recorder.Summary(graph.V);
	}

    protected:
	config::SimulationConfig &cfg;

	Graph<Attr>                       graph;
	std::vector<int>                  targets;
	utils::stats::Recorder<RecordedT> recorder;
	std::vector<std::pair<int, int>>  bid_links;
};
} // namespace Simulator

#endif

#ifndef RCN_SIMULATION_HPP
#define RCN_SIMULATION_HPP

#include <numeric>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "core/config.hpp"
#include "graph/graph.hpp"
#include "simulator/simulator.hpp"

#include "protocol/bgp-rcn/bgp-rcn.hpp"
#include "protocol/bgp-rcn/state/bgp-rcn-attribute.hpp"

#include "utils/helpers/helpers.hpp"
#include "utils/stats/csv-writer.hpp"

using namespace Protocol::RCN;
using namespace Graphs;

namespace Simulator
{

template <class RA> class RCNSimulation : public ISimulation {
    public:
	using base_attr = RA::BaseAttr;
	using base_ext	= RA::Ext;
	using base_ord	= RA::Ord;

	using attr = RCNAttribute<base_attr, base_ext, base_ord>;

	RCNSimulation(config::SimulationConfig		       &cfg_,
		      const std::map<std::string, std::string> &map)
		: cfg(cfg_)
		, graph(Graph<base_attr>::readGraph(cfg.network_file))
		, recorder(cfg.network_file, "rcn", map)
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

		if (cfg.u != -1 && cfg.v != -1) {
			bid_links.push_back({ cfg.u,
					      cfg.v,
					      { graph.adjListOut[cfg.u].at(cfg.v) },
					      { graph.adjListOut[cfg.v].at(cfg.u) } });
		} else {
			bid_links = graph.findBidLinks();
		}
	}

	void run() override
	{
		for (int t : targets) {
			if (cfg.scenario == config::ScenarioType::Normal) {
				recorder.recordDest(t, proto(t));

			} else if (cfg.scenario == config::ScenarioType::LinkFailure) {
				for (const auto &[u, v, edge_uv, edge_vu] : bid_links) {
					recorder.recordDest(
						t, proto(t, u, v), std::make_pair(u, v));

					graph.addEdge(
						u, v, get<0>(edge_uv), get<2>(edge_uv));
					graph.addEdge(
						v, u, get<0>(edge_vu), get<2>(edge_vu));
				}
			}
		}
	}

	utils::stats::SummaryStats postProcess() override
	{
		return recorder.Summary(graph.V);
	}

    private:
	config::SimulationConfig    &cfg;
	Graph<base_attr>	     graph;
	utils::stats::Recorder<attr> recorder;

	std::vector<int> targets;

	std::vector<std::tuple<int,
			       int,
			       std::tuple<double, bool, base_attr>,
			       std::tuple<double, bool, base_attr>>>
		bid_links;

	auto proto(int		      t,
		   std::optional<int> u = std::nullopt,
		   std::optional<int> v = std::nullopt)
	{
		RCN<RA> proto(graph);

		proto.DestinationAnnouncement(t);

		if (u && v) {
			proto.Run();
			proto.cal.schedule(proto.cal.now(), [&]() { proto.NodeDown(t); });

			/*proto.cal.schedule(proto.cal.now(),
					   [&]() {
				proto.LinkFailure(*u, *v); });*/

			proto.Run();
		} else {
			proto.data.log = true;
			proto.Run();
		}

		return proto.data;
	}
};

}

#endif
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
using namespace utils::stats;
using namespace Graphs;

namespace Simulator {
template<class RA>
class RCNSimulation : public ISimulation
{
  public:
    using BaseAttr = RA::BaseAttr;
    using BaseExt = RA::Ext;
    using BaseOrd = RA::Ord;

    RCNSimulation(config::SimulationConfig& cfg_,
                  const std::map<std::string, std::string>& map)
      : cfg(cfg_)
      , graph(Graph<BaseAttr>::readGraph(cfg.network_file))
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
            bid_links.emplace_back(cfg.u, cfg.v);
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
                for (const auto& [u, v] : bid_links) {
                    recorder.recordDest(
                      t, proto(t, u, v), std::make_pair(u, v));

                    graph.activate(u, v);
                }
            }
        }
    }

    SummaryStats postProcess() override { return recorder.Summary(graph.V); }

  private:
    config::SimulationConfig& cfg;

    Graph<BaseAttr> graph;
    std::vector<int> targets;
    std::vector<std::pair<int, int>> bid_links;
    Recorder<RCNAttribute<BaseAttr, BaseExt, BaseOrd>> recorder;

    Data<RCNAttribute<BaseAttr, BaseExt, BaseOrd>> proto(
      int t,
      std::optional<int> u = std::nullopt,
      std::optional<int> v = std::nullopt)
    {
        RCN<RA> proto(graph);

        proto.DestinationAnnouncement(t);

        if (u && v) {
            proto.Run();
            proto.cal.schedule(100, [&]() { proto.LinkFailure(*u, *v); });
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

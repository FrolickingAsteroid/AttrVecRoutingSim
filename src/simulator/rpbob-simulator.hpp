#ifndef RPBOB_SIMULATION_HPP
#define RPBOB_SIMULATION_HPP

#include "core/config.hpp"
#include "protocol/rpbob/rpbob.hpp"
#include "simulator/simulator.hpp"
#include "utils/helpers/helpers.hpp"
#include "utils/stats/csv-writer.hpp"
#include "utils/ui/logger.hpp"
#include <cstddef>
#include <map>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

using namespace Protocol::Rpbob;
using namespace utils::helpers;
using namespace Graphs;

namespace Simulator {
/**
 *
 * @tparam RA
 */
template<class RA>
class RpBOBSimulation : public ISimulation
{
  public:
    using BaseAttr = RA::BaseAttr;
    using Attr = RA::Attr;
    using BaseExt = RA::BaseExt;
    using BaseOrd = RA::BaseOrd;

    /**
     *
     * @param cfg_
     * @param map
     */
    RpBOBSimulation(config::SimulationConfig& cfg_,
                    const std::map<std::string, std::string>& map)
      : cfg(cfg_)
      , graph(Graph<BaseAttr>::readGraph(cfg.network_file))
      , recorder(cfg.network_file, "rpbob", map)
    {
    }

    void setup() override
    {
        if (cfg.N.has_value()) {
            N.assign(graph.V, *cfg.N);
        }

        if (cfg.destination < 0) {
            targets.resize(graph.V);
            std::iota(targets.begin(), targets.end(), 0);
        } else {
            targets = { cfg.destination };
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
            auto [time, o] =
              DelayDijkstra<BaseAttr, BaseOrd, BaseExt>(graph, t);

            conv_time = ConvergenceTime(time);

            if (!cfg.N.has_value()) {
                N = FindNPerNode<BaseAttr>(*cfg.period, time);
            }

            if (cfg.scenario == config::ScenarioType::Normal) {
                convergence_attr[t] = std::move(o);
                recorder.recordDest(t, proto(t));
            }
            if (cfg.scenario == config::ScenarioType::LinkFailure) {
                for (const auto& [u, v] : bid_links) {
                    graph.deactivate(u, v);

                    auto [opt_time, opt] =
                      DelayDijkstra<BaseAttr, BaseOrd, BaseExt>(graph, t);

                    graph.activate(u, v);

                    updateN(opt_time);
                    convergence_attr[t] = std::move(opt);

                    recorder.recordDest(
                      t, proto(t, u, v), std::make_pair(u, v));
                }
            }
        }
    }

    SummaryStats postProcess() override { return recorder.Summary(graph.V); }

  private:
    double conv_time = 0.0;
    config::SimulationConfig& cfg;                                      // gg

    Graph<BaseAttr> graph;                                              // ggg
    std::vector<int> targets;                                           // ggg
    std::vector<std::size_t> N;                                         // gg
    std::vector<std::pair<int, int>> bid_links;                         // gg

    Recorder<Path<Attr>> recorder;                                      // gg
    std::map<int, std::vector<std::vector<BaseAttr>>> convergence_attr; // g

    void updateN(
      const std::vector<std::vector<std::pair<double, BaseAttr>>>& opt_time)
    {
        if (cfg.N.has_value())
            return;
        auto Ns = FindNPerNode<BaseAttr>(*cfg.period, opt_time);
        for (std::size_t i = 0; i < N.size(); ++i) {
            N[i] = std::max(N[i], Ns[i]);
        }
    }

    double ConvergenceTime(
      const std::vector<std::vector<std::pair<double, BaseAttr>>>& opt_time)
    {
        double max_delay = 0.0;
        for (const auto& node_paths : opt_time) {
            if (!node_paths.empty())
                max_delay = std::max(max_delay, node_paths.back().first);
        }
        return max_delay + 1;
    }

    Data<Path<Attr>> proto(int t,
                           std::optional<int> u = std::nullopt,
                           std::optional<int> v = std::nullopt)
    {
        RPBOB<RA> proto(graph, N, cfg.notif);

        proto.nodeConverged = [&](int i) {
            if (i == t)
                return true;
            return proto.s[i].GetBestAttr().attr ==
                   convergence_attr[t][i].front();
        };

        proto.converged = [&]() {
            for (int i = 0; i < graph.V; ++i) {
                if (!proto.nodeConverged(i))
                    return;
            }
            if (!cfg.max.has_value() || cfg.max.value() == 0) {
                proto.cal.clear();
            }
        };

        if (u && v) {
            proto.DestinationAnnouncement(t, 0, *cfg.period, cfg.max);
            proto.cal.schedule(100, [&]() { proto.LinkFailure(*u, *v); });
            proto.Run();
            graph.activate(*u, *v);
        } else {
            proto.data.log = true;
            proto.DestinationAnnouncement(t, 0, *cfg.period, cfg.max);
            proto.Run();
        }

        return proto.data;
    }
};
} // namespace Simulator

#endif
#ifndef RPBOB_SIMULATION_HPP
#define RPBOB_SIMULATION_HPP

#include <cstddef>
#include <numeric>
#include <string>

#include "core/config.hpp"

#include "simulator/simulator.hpp"
#include "protocol/rpbob/rpbob.hpp"

#include "utils/ui/logger.hpp"
#include "utils/helpers/helpers.hpp"
#include "utils/stats/csv-writer.hpp"

using namespace Protocol::Rpbob;
using namespace Graphs;

namespace Simulator {
    template<class RA>
    class RpBOBSimulation : public ISimulation {
    public:
        using base_attr = RA::BaseAttr;
        using base_ext = RA::BaseExt;
        using base_ord = RA::BaseOrd;

        RpBOBSimulation(config::SimulationConfig &cfg_,
                        const std::map<std::string, std::string> &map)
            : cfg(cfg_)
              , graph(Graph<typename RA::BaseAttr>::readGraph(cfg.network_file))
              , recorder(cfg.network_file, "rpbob", map) {
        }

        void setup() override {
            if (cfg.N.has_value()) {
                N.assign(graph.V, *cfg.N);
            }

            if (cfg.destination < 0) {
                targets.resize(graph.V);
                std::iota(targets.begin(), targets.end(), 0);
            } else {
                targets = {cfg.destination};
            }

            if (cfg.u != -1 && cfg.v != -1) {
                bid_links.push_back({
                    cfg.u,
                    cfg.v,
                    {graph.adjListOut[cfg.u].at(cfg.v)},
                    {graph.adjListOut[cfg.v].at(cfg.u)}
                });
            } else {
                bid_links = graph.findBidLinks();
            }
        }

        void run() override {
            for (int t: targets) {
                auto [time, o] =
                        utils::helpers::DelayDijkstra<base_attr, base_ord, base_ext>(graph,
                            t);

                conv_time = ConvergenceTime(time);

                if (!cfg.N.has_value()) {
                    N = utils::helpers::FindNPerNode<base_attr>(*cfg.period, time);
                }

                if (cfg.scenario == config::ScenarioType::Normal) {
                    convergence_attr[t] = std::move(o);
                    recorder.recordDest(t, proto(t));
                }
                if (cfg.scenario == config::ScenarioType::LinkFailure) {
                    for (const auto &[u, v, edge_uv, edge_vu]: bid_links) {
                        graph.deactivate(u, v);

                        auto [opt_time, opt] =
                                utils::helpers::DelayDijkstra<base_attr,
                                    base_ord,
                                    base_ext>(graph, t);

                        graph.activate(u, v);

                        updateN(opt_time);
                        convergence_attr[t] = std::move(opt);

                        recorder.recordDest(
                            t, proto(t, u, v), std::make_pair(u, v));
                    }
                }
            }
        }

        utils::stats::SummaryStats postProcess() override {
            return recorder.Summary(graph.V);
        }

    private:
        std::vector<size_t> N;
        config::SimulationConfig &cfg;
        Graph<base_attr> graph;
        std::vector<int> targets;
        utils::stats::Recorder<Path<typename RA::Attr> > recorder;

        std::map<int, std::vector<std::vector<base_attr> > > convergence_attr;
        double conv_time = 0.0;

        std::vector<std::tuple<int,
            int,
            std::tuple<double, bool, base_attr>,
            std::tuple<double, bool, base_attr> > >
        bid_links;

        void updateN(const std::vector<std::vector<std::pair<double, base_attr> > > &opt_time) {
            if (cfg.N.has_value())
                return;
            auto Ns = utils::helpers::FindNPerNode<base_attr>(*cfg.period, opt_time);
            for (std::size_t i = 0; i < N.size(); ++i)
                N[i] = std::max(N[i], Ns[i]);
        }

        double
        ConvergenceTime(const std::vector<std::vector<std::pair<double, base_attr> > > &opt_time) {
            double max_delay = 0.0;
            for (const auto &node_paths: opt_time)
                if (!node_paths.empty())
                    max_delay = std::max(max_delay, node_paths.back().first);
            return max_delay + 1;
        }

        auto proto(int t, std::optional<int> u = std::nullopt, std::optional<int> v = std::nullopt) {
            RPBOB<RA> proto(graph, N, cfg.notif);

            proto.nodeConverged = [&](int i) {
                if (i == t)
                    return true;
                return proto.s[i].GetBestAttr().attr == convergence_attr[t][i].front(); //needs to change for l + delim
            };

            proto.converged = [&]() {
                if (proto.final_state == true) {
                    return;
                }
                for (int i = 0; i < graph.V; ++i)
                    if (!proto.nodeConverged(i))
                        return;

                if (!cfg.max.has_value()) {
                    proto.cal.clear();
                } else {
                    proto.final_state = true;
                }
            };

            if (u && v) {
                proto.DestinationAnnouncement(t, 0, *cfg.period, *cfg.max);
                proto.cal.schedule(100, [&]() { proto.LinkFailure(*u, *v); });
                proto.Run();
                graph.activate(*u, *v);
            } else {
                proto.data.log = true;
                proto.DestinationAnnouncement(t, 0, *cfg.period, *cfg.max);
                proto.Run();
            }

            return proto.data;
        }
    };
}

#endif

#ifndef BGP_SIMULATION_HPP
#define BGP_SIMULATION_HPP

#include <numeric>
#include <string>

#include "protocol/bgp/bgp.hpp"
#include "simulator/simulator.hpp"

#include "core/config.hpp"
#include "utils/ui/logger.hpp"
#include "utils/helpers/helpers.hpp"
#include "utils/stats/csv-writer.hpp"
#include "protocol/bgp/state/bgp-attribute.hpp"

using namespace Protocol::BGP;
using namespace utils::stats;
using namespace Graphs;

namespace Simulator {
    template<class RA>
    class BGPSimulation : public ISimulation {
    public:
        using BaseAttr = RA::BaseAttr;
        using BaseExt = RA::Ext;
        using BaseOrd = RA::Ord;

        BGPSimulation(config::SimulationConfig &cfg_, const std::map<std::string, std::string> &map)
            : graph(Graph<BaseAttr>::readGraph(cfg.network_file))
              , cfg(cfg_)
              , recorder(cfg.network_file, "bgp", map) {
        }

        void setup() override {
            if (cfg.destination < 0) {
                targets.resize(graph.V);
                std::iota(targets.begin(), targets.end(), 0);
            } else {
                targets = {cfg.destination};
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

        void run() override {
            for (int t: targets) {
                if (cfg.scenario == config::ScenarioType::Normal) {
                    recorder.recordDest(t, proto(t));
                } else if (cfg.scenario == config::ScenarioType::LinkFailure) {
                    for (const auto &[u, v]: bid_links) {
                        recorder.recordDest(t, proto(t, u, v), std::make_pair(u, v));
                        graph.activate(u, v);
                    }
                }
            }
        }

        SummaryStats postProcess() override {
            return recorder.Summary(graph.V);
        }

    private:
        Graph<BaseAttr> graph;
        std::vector<int> targets;
        config::SimulationConfig &cfg;
        std::vector<std::pair<int, int> > bid_links;
        Recorder<BGPAttribute<BaseAttr, BaseExt, BaseOrd> > recorder;


        Data<BGPAttribute<BaseAttr, BaseExt, BaseOrd> > proto(int t, std::optional<int> u = std::nullopt,
                                                              std::optional<int> v = std::nullopt) {
            BGP<RA> proto(graph);

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

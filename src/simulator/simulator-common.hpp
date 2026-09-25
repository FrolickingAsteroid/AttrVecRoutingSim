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
std::vector<std::pair<int, int>> buildBidLinks(const config::SimulationConfig &cfg, Graphs::Graph<Attr> &graph)
{
	if (cfg.u != -1 && cfg.v != -1)
	{
		return {
		    {cfg.u, cfg.v}
                };
	}
	return graph.findBidLinks();
}

inline std::vector<std::vector<std::pair<int, int>>> readCutsets(const std::string &network_file)
{
	auto        slash_pos = network_file.rfind('/');
	std::string dir       = (slash_pos == std::string::npos) ? "." : network_file.substr(0, slash_pos);

	std::string   cutset_path = dir + "/cutset.tsv";
	std::ifstream cutset_file(cutset_path);

	if (!cutset_file.is_open())
	{
		throw std::runtime_error("Could not open network file: " + cutset_path);
	}
	std::vector<std::vector<std::pair<int, int>>> cutsets;

	std::string line;
	while (std::getline(cutset_file, line))
	{
		if (line.empty())
		{
			continue;
		}

		std::vector<std::pair<int, int>> cutset;
		std::istringstream               iss(line);
		std::string                      edge_str;
		while (iss >> edge_str)
		{
			auto comma = edge_str.find(',');
			int  u     = std::stoi(edge_str.substr(0, comma));
			int  v     = std::stoi(edge_str.substr(comma + 1));
			cutset.emplace_back(u, v);
		}
		cutsets.push_back(std::move(cutset));
	}

	return cutsets;
}

template <class Attr, class RecordedT>
class SimulationCommon : public ISimulation
{
    public:
	SimulationCommon(config::SimulationConfig &cfg_, const std::map<std::string, std::string> &map, std::string tag)
	    : cfg(cfg_)
	    , graph(Graphs::Graph<Attr>::readGraph(cfg.network_file))
	    , recorder(cfg.network_file, tag, map, graph.V)
	{
	}

	void setup() override
	{
		targets = buildTargets(cfg, graph.V);

		if (this->cfg.scenario == config::Type::LinkFailure)
		{
			bid_links = buildBidLinks(cfg, graph);
		}
		if (this->cfg.scenario == config::Type::Cutset)
		{
			cutsets = readCutsets(cfg.network_file);
		}
	}

	utils::stats::SummaryStats postProcess() override
	{
		return recorder.Summary();
	}

	void Progress(const int t) const
	{
		const auto dest = static_cast<size_t>(t) + 1;
		if (this->targets.size() == 1)
		{
			utils::ui::printProgressBar(dest, dest);
		}
		else
		{
			utils::ui::printProgressBar(dest, targets.size());
		}
	}

    protected:
	config::SimulationConfig cfg;

	Graphs::Graph<Attr>                           graph;
	std::vector<int>                              targets;
	utils::stats::Recorder<RecordedT>             recorder;
	std::vector<std::pair<int, int>>              bid_links;
	std::vector<std::vector<std::pair<int, int>>> cutsets;
};
} // namespace Simulator

#endif

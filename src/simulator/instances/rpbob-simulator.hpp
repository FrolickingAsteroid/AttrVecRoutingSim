#ifndef RPBOB_SIMULATION_HPP
#define RPBOB_SIMULATION_HPP

#include <optional>
#include <vector>

#include "core/config.hpp"
#include "protocol/rpbob/rpbob.hpp"
#include "protocol/rpbob/state/path-segment.hpp"
#include "simulator/simulator-common.hpp"
#include "utils/helpers/helpers.hpp"

namespace Simulator
{
template <class RA>
class RpBOBSimulation : public SimulationCommon<typename RA::BaseAttr, Protocol::Rpbob::Path<RA>>
{
	using Path = Protocol::Rpbob::Path<RA>;

	using bAttr = RA::BaseAttr;
	using bExt  = RA::BaseExt;
	using bOrd  = RA::BaseOrd;

	using Base = SimulationCommon<bAttr, Path>;

    public:
	using Base::Base;

	void run() override
	{
		for (int t : this->targets)
		{
			auto [time, o] = delayDijkstra(t);
			wind           = utils::helpers::ComputeWindu<bAttr>(*this->cfg.period, time);

			// conv_time = ConvergenceTime(time);

			if (this->cfg.scenario == config::ScenarioType::Normal)
			{
				convergence_attr[t] = std::move(o);
				this->recorder.recordDest(t, proto(t));
			}
			if (this->cfg.scenario == config::ScenarioType::LinkFailure)
			{
				for (const auto &[u, v] : this->bid_links)
				{

					this->graph.deactivate(u, v);
					auto [opt_time, opt] = delayDijkstra(t);
					utils::helpers::updateWindu<bAttr>(*this->cfg.period, wind, opt_time);
					this->graph.activate(u, v);

					convergence_attr[t] = std::move(opt);
					this->recorder.recordDest(t, proto(t, u, v), std::make_pair(u, v));
					this->graph.activate(u, v);
				}
			}
		}
	}

    private:
	double conv_time = 0.0;

	std::vector<std::size_t>                       wind;
	std::map<int, std::vector<std::vector<bAttr>>> convergence_attr;

	auto delayDijkstra(const int t)
	{
		return utils::helpers::DelayDijkstra<RA>(this->graph, t);
	}

	double ConvergenceTime(const std::vector<std::vector<std::pair<double, bAttr>>> &opt_time)
	{
		double max_delay = 0.0;
		for (const auto &node_paths : opt_time)
		{
			if (!node_paths.empty())
				max_delay = std::max(max_delay, node_paths.back().first);
		}
		return max_delay + 1;
	}

	void SetupConv(Protocol::Rpbob::RPBOB<RA> &proto, int t)
	{
		proto.converged = [this, &proto, t]() {
			if (proto.cal.now() > 20)
				int a = 1;
			for (int i = 0; i < this->graph.V; ++i)
			{
				if (i == t)
				{
					continue;
				}
				bool ok = Protocol::Rpbob::CompareOpt(proto.s[i].GetBestAttr(), convergence_attr[t][i]);
				if (!ok)
				{
					return; // 45 still has its old opt val
				}
			}
			if (!this->cfg.max.has_value() || this->cfg.max.value() == 0)
			{
				proto.cal.clear();
			}
		};
	}

	utils::stats::Data<Path> proto(int t, std::optional<int> u = std::nullopt, std::optional<int> v = std::nullopt)
	{
		Protocol::Rpbob::RPBOB<RA> proto(this->graph, wind, this->cfg.notif);
		SetupConv(proto, t);

		proto.DestinationAnnouncement(t, 0, *this->cfg.period, this->cfg.max);

		if (u && v)
		{
			proto.cal.schedule(100, [&]() {
				proto.LinkFailure(*u, *v);
			});
			proto.Run();
		}
		else
		{
			proto.data.log = true;
			proto.Run();
		}

		return proto.data;
	}
};
} // namespace Simulator

#endif

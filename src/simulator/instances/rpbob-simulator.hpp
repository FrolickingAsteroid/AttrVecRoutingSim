#ifndef RPBOB_SIMULATION_HPP
#define RPBOB_SIMULATION_HPP

#include <optional>
#include <vector>

#include "core/config.hpp"
#include "protocol/rpbob/rpbob.hpp"
#include "protocol/rpbob/state/rpbob-attribute.hpp"
#include "simulator/simulator-common.hpp"
#include "utils/helpers/helpers.hpp"

namespace Simulator
{
template <class RA>
class RpBOBSimulation : public SimulationCommon<typename RA::Base::Attr, Protocol::Rpbob::RPBOBAttribute<RA>>
{
	using RPBOBAttribute = Protocol::Rpbob::RPBOBAttribute<RA>;
	using bAttr          = RA::Base::Attr;
	using Base           = SimulationCommon<bAttr, RPBOBAttribute>;

    public:
	using Base::Base;

	void run() override
	{
		for (int t : this->targets)
		{
			auto [time, o] = delayDijkstra(t);

			if (!this->cfg.notif)
			{
				wind = utils::helpers::ComputeWindu<bAttr>(*this->cfg.period, time);
			}

			if (this->cfg.scenario == config::Type::Normal)
			{
				convergence_attr[t] = std::move(o);
				this->recorder.recordDest(t, proto(t));
			}
			if (this->cfg.scenario == config::Type::LinkFailure)
			{
				for (const auto &[u, v] : this->bid_links)
				{
					this->graph.deactivate(u, v);
					auto [opt_time, opt] = delayDijkstra(t);
					this->graph.activate(u, v);

					convergence_attr[t] = std::move(opt);

					if (!this->cfg.notif)
					{
						utils::helpers::updateWindu<bAttr>(*this->cfg.period, wind, opt_time);
					}

					this->recorder.recordDest(t, proto(t, u, v), std::make_pair(u, v));
					this->graph.activate(u, v);
				}
			}
			this->Progress(t);
		}
	}

    private:
	double conv_time = 0.0;

	std::vector<int>                               wind{};
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
			for (int i = 0; i < this->graph.V; ++i)
			{
				if (i == t)
				{
					continue;
				}
				const bool ok = utils::helpers::CompareOpt<RPBOBAttribute, bAttr>(
				    proto.s[i].GetOpt(), convergence_attr[t][i]);

				if (!ok)
				{
					return;
				}
			}
			if (!this->cfg.max.has_value() || this->cfg.max.value() == 0)
			{
				proto.cal.clear();
			}
		};
	}

	utils::stats::Data<RPBOBAttribute> proto(int t, std::optional<int> u = std::nullopt,
						 std::optional<int> v = std::nullopt)
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

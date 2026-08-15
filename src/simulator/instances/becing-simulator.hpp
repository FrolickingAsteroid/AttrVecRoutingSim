#ifndef BECING_SIMULATION_HPP
#define BECING_SIMULATION_HPP

#include <numeric>
#include <string>

#include "core/config.hpp"
#include "protocol/beaconing/beaconing.hpp"
#include "simulator/simulator-common.hpp"
#include "utils/helpers/helpers.hpp"

namespace Simulator
{
template <class RA>
class BCNSimulation : public SimulationCommon<typename RA::Base::Attr, Protocol::Beaconing::Beacon<RA>>
{
	using Bcn   = Protocol::Beaconing::Beacon<RA>;
	using bAttr = RA::Base::Attr;
	using Base  = SimulationCommon<bAttr, Bcn>;

    public:
	using Base::Base;

	void run() override
	{
		for (int t : this->targets)
		{
			auto [time, o] = utils::helpers::DelayDijkstra<RA>(this->graph, t);
			if (this->cfg.scenario == config::Type::Normal)
			{
				convergence_attr[t] = std::move(o);
				this->recorder.recordDest(t, proto(t));
			}
		}
	}

    private:
	double                                         convergence_time = 0;
	std::map<int, std::vector<std::vector<bAttr>>> convergence_attr;

	void SetupConv(Protocol::Beaconing::BEACONING<RA> &proto, int t)
	{
		proto.converged = [this, &proto, t]() {
			for (int i = 0; i < this->graph.V; ++i)
			{
				if (i == t)
				{
					continue;
				}
				const bool ok =
				    utils::helpers::CompareOpt<Bcn, bAttr>(proto.s[i].GetOpt(), convergence_attr[t][i]);

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

	utils::stats::Data<Bcn> proto(int t, std::optional<int> u = std::nullopt, std::optional<int> v = std::nullopt)
	{
		Protocol::Beaconing::BEACONING<RA> proto(this->graph);
		// SetupConv(proto, t);

		proto.DestinationAnnouncement(t, 0, *this->cfg.period, this->cfg.max);

		// periodic propagation on every node,
		// independent of destination announcement
		for (int x = 0; x < this->graph.V; ++x)
		{
			proto.SchedulePropagation(u, *this->cfg.dprop, *this->cfg.max);
		}

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
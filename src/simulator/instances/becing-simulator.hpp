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
class BCNSimulation : public SimulationCommon<typename RA::Attr, Protocol::Beaconing::Beacon<RA>>
{
	using BeaconingAttribute = Protocol::Beaconing::Beacon<RA>;
	using bAttr              = RA::Attr;
	using Base               = SimulationCommon<bAttr, BeaconingAttribute>;

    public:
	using Base::Base;

	void run() override
	{
		for (int t : this->targets)
		{
			auto [time, o] = utils::helpers::DelayDijkstra<RA>(this->graph, t);
			if (this->cfg.scenario == config::Type::Normal)
			{
				// convergence_attr[t] = std::move(o);
				this->recorder.recordDest(t, proto(t));
			}
		}
	}

    private:
	double                                         convergence_time = 0;
	std::map<int, std::vector<std::vector<bAttr>>> convergence_attr;

	void SetupCallback(Protocol::Beaconing::BEACONING<RA> &proto, int t)
	{
		proto.record = [this, t](int u, double time, const std::vector<Protocol::Beaconing::Beacon<RA>> &best,
					 const std::vector<Protocol::Beaconing::Beacon<RA>> &candidates,
					 std::string                                         tag) {
			this->recorder.recordProgress(t, u, time, best, candidates, tag);
		};
	}

	utils::stats::Data<BeaconingAttribute> proto(int t, std::optional<int> u = std::nullopt,
						     std::optional<int> v = std::nullopt)
	{
		Protocol::Beaconing::BEACONING<RA> proto(this->graph);
		SetupCallback(proto, t);

		proto.DestinationAnnouncement(t, 0, *this->cfg.period, this->cfg.max);

		// the last announcement is issued at (max - 1) * period and its beacons expire
		// one expiration interval later; no propagation can send anything afterwards
		const double horizon = (*this->cfg.max - 1) * *this->cfg.period + BeaconingAttribute::expiration;

		// periodic propagation on every node,
		// independent of destination announcement
		for (int x = 0; x < this->graph.V; ++x)
		{
			if (x == t)
			{
				continue;
			}
			proto.SchedulePropagation(x, *this->cfg.dprop, horizon);
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
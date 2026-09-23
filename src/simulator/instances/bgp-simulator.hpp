#ifndef BGP_SIMULATION_HPP
#define BGP_SIMULATION_HPP

#include "core/config.hpp"
#include "protocol/bgp/bgp.hpp"
#include "protocol/bgp/state/bgp-attribute.hpp"
#include "simulator/simulator-common.hpp"

namespace Simulator
{
template <class RA>
class BGPSimulation : public SimulationCommon<typename RA::Attr, Protocol::BGP::BGPAttribute<RA>>
{
	using BGPAttribute = Protocol::BGP::BGPAttribute<RA>;

	using Base = SimulationCommon<typename RA::Attr, BGPAttribute>;

    public:
	using Base::Base;

	void run() override
	{
		for (int t : this->targets)
		{
			if (this->cfg.scenario == config::Type::Normal)
			{
				this->recorder.recordDest(t, proto(t));
			}
			else if (this->cfg.scenario == config::Type::LinkFailure)
			{
				for (const auto &[u, v] : this->bid_links)
				{
					this->recorder.recordDest(t, proto(t, u, v), std::make_pair(u, v));
					this->graph.activate(u, v);
				}
			}
			else if (this->cfg.scenario == config::Type::Withdrawal)
			{
				for (int u = 0; u < this->graph.V; ++u)
				{
					if (u == t)
					{
						continue;
					}

					this->recorder.recordDest(t, proto(t, u), std::make_pair(u, -1));
					this->graph.activate(u);
				}
			}
			this->Progress(t);
		}
	}

    private:
	utils::stats::Data<BGPAttribute> proto(int t, std::optional<int> u = std::nullopt,
					       std::optional<int> v = std::nullopt)
	{
		Protocol::BGP::BGP<RA> proto(this->graph);

		proto.DestinationAnnouncement(t);

		if (u && v)
		{
			proto.Run();
			proto.cal.schedule(200, [&]() {
				proto.LinkFailure(*u, *v);
			});
			proto.Run();
		}
		else if (u)
		{
			proto.Run();
			proto.cal.schedule(100, [&]() {
				proto.NodeWithdraw(*u);
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

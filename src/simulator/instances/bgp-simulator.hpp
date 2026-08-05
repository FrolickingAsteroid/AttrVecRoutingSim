#ifndef BGP_SIMULATION_HPP
#define BGP_SIMULATION_HPP

#include "core/config.hpp"
#include "protocol/bgp/bgp.hpp"
#include "protocol/bgp/state/bgp-attribute.hpp"
#include "simulator/simulator-common.hpp"

namespace Simulator
{
template <class RA>
class BGPSimulation
    : public SimulationCommon<typename RA::BaseAttr,
			      Protocol::BGP::BGPAttribute<typename RA::BaseAttr, typename RA::Ext, typename RA::Ord>>
{
	using BaseAttr = RA::BaseAttr;
	using BaseExt  = RA::Ext;
	using BaseOrd  = RA::Ord;

	using BGPAttribute = Protocol::BGP::BGPAttribute<BaseAttr, BaseExt, BaseOrd>;

	using Base = SimulationCommon<BaseAttr, BGPAttribute>;

    public:
	using Base::Base;

	void run() override
	{
		for (int t : this->targets)
		{
			if (this->cfg.scenario == config::ScenarioType::Normal)
			{
				this->recorder.recordDest(t, proto(t));
			}
			else if (this->cfg.scenario == config::ScenarioType::LinkFailure)
			{
				for (const auto &[u, v] : this->bid_links)
				{
					this->recorder.recordDest(t, proto(t, u, v), std::make_pair(u, v));
					this->graph.activate(u, v);
				}
			}
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
			proto.cal.schedule(100, [&]() { proto.LinkFailure(*u, *v); });
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

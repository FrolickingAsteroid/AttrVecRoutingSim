#ifndef RCN_SIMULATION_HPP
#define RCN_SIMULATION_HPP

#include "core/config.hpp"
#include "protocol/bgp-rcn/bgp-rcn.hpp"
#include "protocol/bgp-rcn/state/bgp-rcn-attribute.hpp"

namespace Simulator
{
template <class RA>
class RCNSimulation
    : public SimulationCommon<typename RA::BaseAttr,
			      Protocol::RCN::RCNAttribute<typename RA::BaseAttr, typename RA::Ext, typename RA::Ord>>
{
	using BaseAttr = RA::BaseAttr;
	using BaseExt  = RA::Ext;
	using BaseOrd  = RA::Ord;

	using RCNAttribute = Protocol::RCN::RCNAttribute<BaseAttr, BaseExt, BaseOrd>;

	using Base = SimulationCommon<BaseAttr, RCNAttribute>;

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
		}
	}

    private:
	utils::stats::Data<RCNAttribute> proto(int t, std::optional<int> u = std::nullopt,
					       std::optional<int> v = std::nullopt)
	{
		Protocol::RCN::RCN<RA> proto(this->graph);
		proto.DestinationAnnouncement(t);

		if (u && v)
		{
			proto.Run();
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

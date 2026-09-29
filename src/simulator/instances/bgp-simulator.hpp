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
			else if (this->cfg.scenario == config::Type::Cutset)
			{
				for (const auto &cutset : this->cutsets)
				{
					auto p = proto(t, std::nullopt, std::nullopt, cutset);
					this->recorder.recordDest(t, p, cutset);

					for (const auto &[u, v] : cutset)
					{
						this->graph.activate(u, v);
					}
				}
			}
			this->Progress(t);
		}
	}

    private:
	utils::stats::Data<BGPAttribute> proto(
	    int t, std::optional<int> u = std::nullopt, std::optional<int> v = std::nullopt,
	    const std::optional<std::vector<std::pair<int, int>>> &cutset = std::nullopt)
	{
		Protocol::BGP::BGP<RA> proto(this->graph);

		proto.DestinationAnnouncement(t);

		if (u && v)
		{
			proto.Run();
			proto.cal.schedule(proto.cal.now(), [&]() {
				proto.LinkFailure(*u, *v);
			});
			proto.Run();
		}
		else if (cutset)
		{
			proto.Run();
			for (const auto &[u, v] : *cutset)
			{
				proto.cal.schedule(proto.cal.now(), [&]() {
					proto.LinkFailure(u, v);
				});
			}
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

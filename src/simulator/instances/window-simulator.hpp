#ifndef WINDOW_SIMULATION_HPP
#define WINDOW_SIMULATION_HPP

#include "protocol/rpbob/state/rpbob-attribute.hpp"
#include "simulator/simulator-common.hpp"
#include "utils/helpers/helpers.hpp"

namespace Simulator
{
template <class RA>
class WindowSimulation
    : public SimulationCommon<typename utils::helpers::Base<RA>::Attr, Protocol::Rpbob::RPBOBAttribute<RA>>
{
	using RPBOBAttribute = Protocol::Rpbob::RPBOBAttribute<RA>;
	using bAttr          = RA::Base::Attr;
	using Base           = SimulationCommon<bAttr, RPBOBAttribute>;

    public:
	using Base::Base;

	void run() override
	{
		for (int dest : this->targets)
		{
			if (this->cfg.scenario == config::Type::Normal)
			{
				auto [opt_time, _] = utils::helpers::DelayDijkstra<RA>(this->graph, dest);

				std::vector<long long> wind =
				    utils::helpers::ComputeWindu<bAttr, long long>(*this->cfg.period, opt_time);
				this->recorder.template recordWind<bAttr>(dest, opt_time, wind);
			}
			if (this->cfg.scenario == config::Type::LinkFailure)
			{
				for (const auto &[u, v] : this->bid_links)
				{
					this->graph.deactivate(u, v);
					auto [opt_time, opt] = utils::helpers::DelayDijkstra<RA>(this->graph, dest);
					this->graph.activate(u, v);

					std::vector<long long> wind =
					    utils::helpers::ComputeWindu<bAttr, long long>(*this->cfg.period, opt_time);

					this->recorder.template recordWind<bAttr>(dest, opt_time, wind,
										  std::make_pair(u, v));
				}
			}

			this->Progress(dest);
		}
	}
};
} // namespace Simulator
#endif
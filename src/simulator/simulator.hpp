#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include "utils/stats/data.hpp"

namespace Simulator
{
struct ISimulation {
	virtual ~ISimulation() = default;

	virtual void			   setup()	 = 0;
	virtual void			   run()	 = 0;
	virtual utils::stats::SummaryStats postProcess() = 0;
};

}

#endif

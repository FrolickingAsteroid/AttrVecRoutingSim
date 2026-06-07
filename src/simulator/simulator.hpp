#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

namespace Simulator
{
struct ISimulation {
	virtual ~ISimulation() = default;
	virtual void setup()   = 0;
	virtual void run()     = 0;
};

}

#endif

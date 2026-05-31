#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <string>

#include "utils/register/data.hpp"
#include "graph/graph.hpp"

namespace Simulator
{
struct ISimulation {
	virtual ~ISimulation() = default;
	virtual void setup() = 0;
	virtual void run() = 0;
};

template <class R> struct Simulation : ISimulation {
	utils::Registrar<R> registrar;

	Simulation(const std::string &filename, int max_, int dest_,
		   double period_, utils::RegisterType log_t_,
		   const char *proto_, const std::string &param_name_,
		   const std::string &param_value_)
		: registrar(filename, max_, dest_, period_, log_t_,
			    std::string(proto_), param_name_, param_value_)
	{
	}
};
} // namespace Simulator

#endif // SIMULATOR_HPP

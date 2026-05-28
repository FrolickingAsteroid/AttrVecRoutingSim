#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP
#include <fstream>
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

template <class A, class R> struct Simulation : ISimulation {
	utils::Registrar<R> registrar;

	Simulation(const std::string &filename, int max_, int dest_,
		   double period_, utils::RegisterType log_t_,
		   const char *proto_, const std::string &param_name_,
		   const std::string &param_value_)
		: registrar(filename, max_, dest_, period_, log_t_,
			    std::string(proto_), param_name_, param_value_)
	{
	}

    protected:
	static Graphs::Graph<A> ReadGraph(const std::string &filename)
	{
		std::ifstream fileNetwork(filename);

		if (!fileNetwork.is_open()) {
			throw std::runtime_error(
				"Could not open network file: " + filename);
		}

		int V;
		int u;
		int v;

		double delay;
		A attr;

		fileNetwork >> V;

		Graphs::Graph<A> graph(V);

		while (fileNetwork >> u >> v >> delay >> attr) {
			graph.addEdge(u, v, delay, attr);
		}

		return graph;
	}
};
} // namespace Simulator

#endif // SIMULATOR_HPP

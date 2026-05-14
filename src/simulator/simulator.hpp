#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP
#include <fstream>
#include <string>

#include "graph/graph.hpp"
#include "utils/logger.hpp"

namespace Simulator {
    class Simulation {
    public:
        virtual ~Simulation() = default;

        virtual void setup() = 0;

        virtual void run() = 0;

        //virtual void printResults() const = 0;

    protected:
        template<class A>
        static Graphs::Graph<A> ReadGraph(const std::string &filename) {
            std::ifstream fileNetwork(filename);

            if (!fileNetwork.is_open()) {
                throw std::runtime_error("Could not open network file: " + filename);
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
}// namespace Simulator

#endif// SIMULATOR_HPP

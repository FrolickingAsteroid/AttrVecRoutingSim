#ifndef _GRAPHS_HPP_
#define _GRAPHS_HPP_

#include <unordered_map>
#include <vector>

namespace Graphs {
    template<class S>
    struct Graph {
        int V;
        int E;
        std::vector<std::unordered_map<int, std::tuple<double, bool, S> > > adjListIn;
        std::vector<std::unordered_map<int, std::tuple<double, bool, S> > > adjListOut;

        Graph() {
        }

        explicit Graph(int V_);

        static Graph readGraph(const std::string &filename);

        std::vector<std::pair<int, int> > findBidLinks();

        void addEdge(int, int, double delay, S);

        void removeEdge(int, int);

        void activate(int, int);

        void deactivate(int, int);

        void clear();
    };
}

#include "graph/graph.tpp"

#endif

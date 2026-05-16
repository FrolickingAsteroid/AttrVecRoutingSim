#ifndef _GRAPHS_TPP_
#define _GRAPHS_TPP_

#include <stdexcept>

#include "graph.hpp"

namespace Graphs {
    template<class S>
    Graph<S>::Graph(int V_) : V(V_) {
        this->E = 0;
        adjListIn.resize(V);
        adjListOut.resize(V);
    }

    template<class S>
    void Graph<S>::addEdge(int u, int v, double delay, S w) {
        if (0 > u or u >= V) { throw std::out_of_range("In-Node  does not exist."); }
        if (0 > u or v >= V) { throw std::out_of_range("Out-Node does not exist."); }
        E++;
        adjListIn[v][u] = {delay, true, w};
        adjListOut[u][v] = {delay, true, w};
    }

    template<class S>
    void Graph<S>::removeEdge(int u, int v) {
        if (u < 0 or u >= V) { throw std::out_of_range("In-Node does not exist."); }
        if (v < 0 or v >= V) { throw std::out_of_range("Out-Node does not exist."); }
        E--;
        adjListIn[v].erase(u);
        adjListOut[u].erase(v);
    }

    template<class S>
    void Graph<S>::activate(int u, int v) {
        if (0 > u or u >= V) {
            throw std::out_of_range("Out-Node  does not exist.");
        }
        if (0 > v or v >= V) {
            throw std::out_of_range("In-Node  does not exist.");
        }
        std::get<1>(adjListOut[u].at(v)) = true;
        std::get<1>(adjListOut[v].at(u)) = true;
    }

    template<class S>
    void Graph<S>::deactivate(int u, int v) {
        if (0 > u or u > V) {
            throw std::out_of_range("Out-Node does not exist.");
        }
        if (0 > v or v > V) {
            throw std::out_of_range("In-Node  does not exist.");
        }
        std::get<1>(adjListOut[u].at(v)) = false;
        std::get<1>(adjListOut[v].at(u)) = false;
    }

    template<class S>
    void Graph<S>::clear() {
        V = 0;
        E = 0;
        adjListIn.clear();
        adjListOut.clear();
    }
}
#endif

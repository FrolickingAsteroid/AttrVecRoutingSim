#ifndef HELPERS_HPP
#define HELPERS_HPP

#include "graph/graph.hpp"

namespace utils::helpers
{

template <class A, class Ord, class Ext>
std::size_t Compute_N(const std::string &filename, double Delta);

template <class A, class Ord>
double
WorstDelay(const std::vector<std::vector<std::pair<double, A> > > &opt_time);

template <class A, class Ord, class Ext>
std::vector<std::vector<std::pair<double, A> > >
DelayDijkstra(const Graphs::Graph<A> &graph, int dest);

void printDelays(std::vector<std::pair<int, double> > &results,
		 const std::string &filename, double Delta);

}

#include "utils/helpers/helpers.tpp"
#endif
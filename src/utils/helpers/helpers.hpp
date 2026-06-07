#ifndef HELPERS_HPP
#define HELPERS_HPP

#include "graph/graph.hpp"

namespace utils::helpers
{

template <class A, class Ord, class Ext> void Compute_N(const std::string &filename, double Delta);

template <class A, class Ord, class Ext>
void Compute_N(const std::string &filename, double Delta, int t);

template <class A, class Ord, class Ext>
std::pair<std::vector<std::vector<std::pair<double, A>>>,
	  std::vector<std::vector<std::pair<int, A>>>>
DelayDijkstra(const Graphs::Graph<A> &graph, int dest);

template <class A, class Ord>
std::pair<double, std::vector<double>>
WorstDelay(const std::vector<std::vector<std::pair<double, A>>> &opt_time);

template <class A>
std::vector<int> propagateN(const std::vector<double> &node_D,
				    const Graphs::Graph<A> &graph, double Delta);

void printDelays(std::vector<double> &results, double D);

} // namespace utils::helpers

#include "utils/helpers/helpers.tpp"
#endif
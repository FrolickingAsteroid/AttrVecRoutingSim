#ifndef HELPERS_HPP
#define HELPERS_HPP

#include "graph/graph.hpp"

namespace utils::helpers
{

template <class A>
std::vector<std::size_t> ComputeWindu(double Delta, const std::vector<std::vector<std::pair<double, A>>> &opt_time);

template <class A>
void updateWindu(double Delta, std::vector<std::size_t> &wind,
		 const std::vector<std::vector<std::pair<double, A>>> &opt_time);

template <class A> double FindMaxDelayDiff(const std::vector<std::vector<std::pair<double, A>>> &opt_time);

template <class A, class Ord, class Ext>
std::pair<std::vector<std::vector<std::pair<double, A>>>, std::vector<std::vector<A>>> DelayDijkstra(
    const Graph<A> &graph, int dest);

void printDelays(std::vector<double> &results, double D);

/*
template <class A, class Ord, class Ext> void Compute_N(const std::string &filename, double Delta);

template <class A, class Ord, class Ext>
void Compute_N(const std::string &filename, double Delta, int t);

template <class A, class Ord>
std::pair<double, std::vector<double>>
WorstDelay(const std::vector<std::vector<std::pair<double, A>>> &opt_time);

template <class A>
std::vector<int>
propagateN(const std::vector<double> &node_D, const Graphs::Graph<A> &graph, double Delta);

void printDelays(std::vector<double> &results, double D);
*/
} // namespace utils::helpers

#include "utils/helpers/helpers.tpp"
#endif

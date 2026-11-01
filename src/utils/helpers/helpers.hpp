#ifndef HELPERS_HPP
#define HELPERS_HPP

#include "graph/graph.hpp"

namespace utils::helpers
{

template <class RA>
auto BaseHelper()
{
	if constexpr (requires { typename RA::Base; })
	{
		return std::type_identity<typename RA::Base>{};
	}
	else
	{
		return std::type_identity<RA>{};
	}
}

template <class RA>
using Base = decltype(BaseHelper<RA>())::type;

// N: integer type of the window sizes. The window simulator uses long long: on topologies with exponential path
// delays (focus) a window at small periods exceeds what an int holds
template <class A, class N = int>
std::vector<N> ComputeWindu(double Delta, const std::vector<std::vector<std::pair<double, A>>> &opt_time);

template <class A>
void updateWindu(double Delta, std::vector<int> &wind, const std::vector<std::vector<std::pair<double, A>>> &opt_time);

template <class A>
double FindMaxDelayDiff(const std::vector<std::vector<std::pair<double, A>>> &opt_time);

template <class RA>
std::pair<std::vector<std::vector<std::pair<double, typename Base<RA>::Attr>>>,
	  std::vector<std::vector<typename Base<RA>::Attr>>>
DelayDijkstra(const Graphs::Graph<typename Base<RA>::Attr> &graph, int dest);

void printDelays(const std::vector<double> &results, double D);

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

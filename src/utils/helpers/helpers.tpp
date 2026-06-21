#ifndef HELPERS_TPP
#define HELPERS_TPP

#include "utils/helpers/helpers.hpp"
#include "utils/ui/appearance.hpp"
#include "utils/ui/logger.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <set>
#include <vector>

using namespace utils::ui;

namespace utils::helpers
{

template <class A>
std::vector<std::size_t> FindNPerNode(double						    Delta,
				      const std::vector<std::vector<std::pair<double, A>>> &opt_time)
{
	std::vector<std::size_t> Ns;
	Ns.reserve(opt_time.size());
	for (const auto &node_paths : opt_time) {
		if (Delta == 0 || node_paths.size() < 2) {
			Ns.push_back(0);
			continue;
		}
		double diff = node_paths.back().first - node_paths.front().first;
		Ns.push_back(static_cast<std::size_t>(std::ceil(diff / Delta)));
	}
	return Ns;
}

template <class A> double FindMaxDelayDiff(const std::vector<std::vector<std::pair<double, A>>> &opt_time)
{
	double max_diff = 0.0;
	for (const auto &node_paths : opt_time) {
		if (node_paths.size() < 2)
			continue;
		double diff = node_paths.back().first - node_paths.front().first;
		max_diff    = std::max(max_diff, diff);
	}
	return max_diff;
}

template <class A, class Ord, class Ext>
std::pair<std::vector<std::vector<std::pair<double, A>>>, std::vector<std::vector<A>>>
DelayDijkstra(const Graph<A> &graph, int dest)
{
	Ord ord;
	Ext ext;

	const int V = graph.V;

	std::vector<std::vector<A>>		       opt(V);
	std::vector<std::vector<std::pair<double, A>>> opt_time(V);

	using Quad = std::tuple<double, int, A, int>; // (tau, node, attr, succ)
	auto comp  = [](const Quad &lhs, const Quad &rhs) { return std::get<0>(lhs) > std::get<0>(rhs); };

	auto in = [](const A &a, const std::vector<A> &set) {
		return std::find_if(set.begin(), set.end(), [&](const A &p) { return p == a; }) != set.end();
	};

	auto select = [&](std::vector<A> &set, const A &a) -> bool {
		for (const auto &p : set)
			if (ord(p, a))
				return false;
		set.erase(std::remove_if(set.begin(), set.end(), [&](const A &p) { return ord(a, p); }),
			  set.end());
		set.push_back(a);
		return true;
	};

	std::priority_queue<Quad, std::vector<Quad>, decltype(comp)> pq(comp);

	A neutral = Ord::Neutral();
	pq.push(std::make_tuple(0.0, dest, neutral, -1));
	std::set<int> visited;

	while (!pq.empty()) {
		auto [tau, u, a, succ] = pq.top();
		pq.pop();

		if (!in(a, opt[u])) {
			if (select(opt[u], a)) {
				opt_time[u].push_back({ tau, a });
				for (auto &[x, edge] : graph.adjListIn[u]) {
					if (x == dest) {
						continue;
					}
					auto &[delay, active, link_attr] = edge;
					A candidate			 = ext(link_attr, a);
					pq.push(std::make_tuple(tau + delay, x, candidate, u));
				}
			}
		}
	}
	return { opt_time, opt };
}

template <class A, class Ord, class Ext> void Compute_N(const std::string &filename, double Delta)
{
	auto   graph = Graph<A>::readGraph(filename);
	double D     = 0.0;

	LOG_INFO << ui::cyan("\nWorst delay per destination — " + filename + " (" + std::to_string(graph.V) +
			     " destinations)" + "\n");

	std::vector<double> results;
	for (int u = 0; u < graph.V; ++u) {
		auto [opt_time, opt] = DelayDijkstra<A, Ord, Ext>(graph, u);
		auto delay	     = FindMaxDelayDiff<A>(opt_time);

		D = std::max(D, delay);
		results.push_back(delay);
	}

	printDelays(results, D);

	LOG_INFO << cyan(bold("\nLargest Window size N: ")) << ceil(D / Delta) << "\n";
}

inline void printDelays(std::vector<double> &results, double D)
{
	constexpr int COL_WIDTH = 22;
	const int     total	= static_cast<int>(results.size());
	const int     n_cols	= numColumns(COL_WIDTH);
	const int     n_rows	= numRows(total, n_cols);

	if (total < n_cols * n_rows) {
	}

	tabulate::Table t;

	tabulate::Table::Row_t header;
	for (int c = 0; c < n_cols; ++c) {
		header.push_back("Dest");
		header.push_back("D");
	}
	t.add_row(header);

	std::vector<std::pair<int, int>> cells;
	for (int row = 0; row < n_rows; ++row) {
		tabulate::Table::Row_t r;
		for (int col = 0; col < n_cols; ++col) {
			int idx = dataIndex(row, col, n_rows);
			if (idx < total) {
				r.push_back(std::to_string(idx));
				std::ostringstream ss;
				ss << std::fixed << std::setprecision(2) << results[idx];
				r.push_back(ss.str());
				if (D == results[idx]) {
					cells.push_back({ row, col });
				}
			} else {
				r.push_back("");
				r.push_back("");
			}
		}
		t.add_row(r);
	}

	for (int c = 0; c < n_cols; ++c)
		t.column(c * 2).format().font_color(tabulate::Color::cyan);

	t[0].format()
		.font_style({ tabulate::FontStyle::bold })
		.font_color(tabulate::Color::yellow)
		.font_align(tabulate::FontAlign::center);

	for (auto &[row, col] : cells)
		t[row + 1][col * 2 + 1]
			.format()
			.font_color(tabulate::Color::green)
			.font_style({ tabulate::FontStyle::bold });

	LOG_INFO << t << "\n";
}

/*
template <class A, class Ord>
std::pair<double, std::vector<double>>
WorstDelay(const std::vector<std::vector<std::pair<double, A>>> &opt_time)
{
	Ord		    ord;
	double		    D = 0.0;
	std::vector<double> node_D(opt_time.size(), 0.0);

	for (std::size_t u = 0; u < opt_time.size(); ++u) {
		const auto &pairs = opt_time[u];
		if (pairs.size() < 2)
			continue;
		for (const auto &[ti, ai] : pairs) {
			for (const auto &[tj, aj] : pairs) {
				if (ai == aj)
					continue;
				if (ord(ai, aj)) {
					node_D[u] = std::max(node_D[u], ti - tj);
				}
			}
		}
		D = std::max(D, node_D[u]);
	}
	return { D, node_D };
}
*/

/*
template <class A, class Ord, class Ext>
void Compute_N(const std::string &filename, double Delta, int t)
{
	LOG_INFO << ui::cyan("\nWorst delay per destination — " + filename + " (destination " +
			     std::to_string(t) + ")\n\n");
	auto graph	     = Graphs::Graph<A>::readGraph(filename);
	auto [opt_time, opt] = DelayDijkstra<A, Ord, Ext>(graph, t);
	auto [D, node_D]     = WorstDelay<A, Ord>(opt_time);

	printDelays(node_D, D);

	LOG_INFO << cyan(bold("\nLargest Window size N: ")) << ceil(D / Delta) << "\n";

	propagateN(node_D, graph, Delta);
}


template <class A>
std::vector<int>
propagateN(const std::vector<double> &node_D, const Graphs::Graph<A> &graph, double Delta)
{
	std::vector<int> node_N(graph.V);
	for (int u = 0; u < graph.V; ++u)
		node_N[u] = static_cast<int>(std::ceil(node_D[u] / Delta));

	int max = std::max_element(node_N.begin(), node_N.end()) - node_N.begin();

	using P = std::pair<int, int>;
	std::priority_queue<P> pq;
	pq.push({ node_N[max], max });

	while (!pq.empty()) {
		auto [N, v] = pq.top();
		pq.pop();
		for (auto &[u, edge] : graph.adjListIn[v]) {
			if (node_N[u] < N) {
				node_N[u] = N;
				pq.push({ node_N[u], u });
			}
		}
	}

	LOG_DEBUG << "\n  --- N ---";
	for (int u = 0; u < graph.V; ++u)
		LOG_DEBUG << "  node_N[" << u << "] = " << node_N[u];

	LOG_DEBUG << "\n";

	return node_N;
}


*/
} // namespace utils::helpers
#endif
#ifndef HELPERS_TPP
#define HELPERS_TPP

#include <algorithm>
#include <cmath>
#include <queue>
#include <set>
#include <vector>

#include "routing-algebras/orders/comparison-vocab.hpp"
#include "utils/helpers/helpers.hpp"
#include "utils/ui/appearance.hpp"
#include "utils/ui/logger.hpp"

using namespace utils::ui;

namespace utils::helpers
{

template <class Msg>
bool CompareOpt(const std::vector<Msg> &l, const std::vector<Msg> &r)
{
	if (l.size() != r.size())
	{
		return false;
	}
	for (const auto &l_ : l)
	{
		bool found = std::any_of(r.begin(), r.end(), [&](const Msg &r_) {
			return l_.attr.getAttr() == r_.attr.getAttr();
		});
		if (!found)
		{
			return false;
		}
	}
	return true;
}

template <class Msg, class A>
bool CompareOpt(const std::vector<Msg> &l, const std::vector<A> &r)
{
	if (r.empty() && l.front().attr.getSeqNum() == -1)
	{
		return true;
	}
	if (l.size() != r.size())
	{
		return false;
	}
	for (const auto &l_ : l)
	{
		bool found = std::any_of(r.begin(), r.end(), [&](const A &r_) {
			return l_.attr.getAttr() == r_;
		});
		if (!found)
		{
			return false;
		}
	}
	return true;
}

template <class A, class N>
std::vector<N> ComputeWindu(const double Delta, const std::vector<std::vector<std::pair<double, A>>> &opt_time)
{
	std::vector<N> Ns;
	Ns.reserve(opt_time.size());
	for (const auto &node_paths : opt_time)
	{
		if (Delta == 0 || node_paths.empty())
		{
			// N = 1; W = N + 1,
			// where N = ceil[(fastest - slowest)/Delta] + 1
			Ns.push_back(1);
			continue;
		}
		const double diff = 1.1 * node_paths.back().first - node_paths.front().first;
		Ns.push_back(static_cast<N>(std::ceil(diff / Delta)) + 1);
	}
	return Ns;
}

template <class A>
void updateWindu(const double Delta, std::vector<int> &wind,
		 const std::vector<std::vector<std::pair<double, A>>> &opt_time)
{

	auto new_windu = ComputeWindu<A>(Delta, opt_time);
	for (std::size_t i = 0; i < wind.size(); ++i)
	{
		wind[i] = std::max(wind[i], new_windu[i]);
	}
}

template <class A>
double FindMaxDelayDiff(const std::vector<std::vector<std::pair<double, A>>> &opt_time)
{
	double max_diff = 0.0;
	for (const auto &node_paths : opt_time)
	{
		if (node_paths.size() < 2)
			continue;
		double diff = node_paths.back().first - node_paths.front().first;
		max_diff    = std::max(max_diff, diff);
	}
	return max_diff;
}

template <class RA>
std::pair<std::vector<std::vector<std::pair<double, typename Base<RA>::Attr>>>,
	  std::vector<std::vector<typename Base<RA>::Attr>>>
DelayDijkstra(const Graphs::Graph<typename Base<RA>::Attr> &graph, int dest)
{
	using Base = Base<RA>;
	using A    = Base::Attr;

	typename Base::Ord ord;
	typename Base::Ext ext;

	const int V = graph.V;

	std::vector<std::vector<A>>                    opt(V);
	std::vector<std::vector<std::pair<double, A>>> opt_time(V);

	using Quad = std::tuple<double, int, A, int>; // (tau, node, attr, predecessor)

	auto comp = [](const Quad &lhs, const Quad &rhs) {
		return std::get<0>(lhs) > std::get<0>(rhs);
	};

	auto in = [](const A &a, const std::vector<A> &set) {
		return std::find_if(set.begin(), set.end(), [&](const A &p) {
			return p == a;
		}) != set.end();
	};

	auto select = [&](std::vector<A> &set, const A &a) -> bool {
		for (const auto &p : set)
		{

			if (RoutingAlgebras::BetterOrEqual(ord(p, a)))
			{
				return false;
			}
		}
		set.erase(std::remove_if(set.begin(), set.end(),
					 [&](const A &p) {
			return BetterOrEqual(ord(a, p));
		}),
			  set.end());
		set.push_back(a);
		return true;
	};

	std::priority_queue<Quad, std::vector<Quad>, decltype(comp)> pq(comp);

	A neutral = Base::Neutral(dest);
	pq.push(std::make_tuple(0.0, dest, neutral, -1));

	std::set<int> visited;
	while (!pq.empty())
	{
		auto [tau, u, a, succ] = pq.top();
		pq.pop();
		if (!in(a, opt[u]))
		{
			if (select(opt[u], a))
			{
				opt_time[u].push_back({tau, a});
				for (auto &[x, edge] : graph.adjListIn[u])
				{
					if (x == dest)
					{
						continue;
					}
					auto &[delay, active, link_weight] = edge;
					if (!active)
					{
						continue;
					}
					auto l = link_weight.buildLinkAttr(x);

					A candidate = ext(a, l);

					pq.push(std::make_tuple(tau + delay, x, candidate, u));
				}
			}
		}
	}
	return {opt_time, opt};
}

template <class RA>
void Compute_N(const std::string &filename, double Delta, const Graphs::Graph<typename Base<RA>::Attr> &graph,
	       const std::vector<std::vector<std::pair<double, typename Base<RA>::Attr>>> &opt_time)
{
	double D = 0.0;

	LOG_INFO << ui::cyan("\nWorst delay per destination — " + filename + " (" + std::to_string(graph.V) +
			     " destinations)" + "\n");

	std::vector<double> results;
	for (int u = 0; u < graph.V; ++u)
	{
		auto delay = FindMaxDelayDiff<typename Base<RA>::Attr>(opt_time);

		D = std::max(D, delay);
		results.push_back(delay);
	}

	printDelays(results, D);

	LOG_INFO << cyan(bold("\nLargest Window size N: ")) << ceil(D / Delta) << "\n";
}

inline void printDelays(const std::vector<double> &results, const double D)
{
	constexpr int COL_WIDTH = 22;
	const int     total     = static_cast<int>(results.size());
	const int     n_cols    = numColumns(COL_WIDTH);
	const int     n_rows    = numRows(total, n_cols);

	if (total < n_cols * n_rows)
	{
	}

	tabulate::Table t;

	tabulate::Table::Row_t header;
	for (int c = 0; c < n_cols; ++c)
	{
		header.push_back("Dest");
		header.push_back("D");
	}
	t.add_row(header);

	std::vector<std::pair<int, int>> cells;
	for (int row = 0; row < n_rows; ++row)
	{
		tabulate::Table::Row_t r;
		for (int col = 0; col < n_cols; ++col)
		{
			int idx = dataIndex(row, col, n_rows);
			if (idx < total)
			{
				r.push_back(std::to_string(idx));
				std::ostringstream ss;
				ss << std::fixed << std::setprecision(2) << results[idx];
				r.push_back(ss.str());
				if (D == results[idx])
				{
					cells.push_back({row, col});
				}
			}
			else
			{
				r.push_back("");
				r.push_back("");
			}
		}
		t.add_row(r);
	}

	for (int c = 0; c < n_cols; ++c)
		t.column(c * 2).format().font_color(tabulate::Color::cyan);

	t[0].format()
	    .font_style({tabulate::FontStyle::bold})
	    .font_color(tabulate::Color::yellow)
	    .font_align(tabulate::FontAlign::center);

	for (auto &[row, col] : cells)
		t[row + 1][col * 2 + 1]
		    .format()
		    .font_color(tabulate::Color::green)
		    .font_style({tabulate::FontStyle::bold});

	LOG_INFO << t << "\n";
}
} // namespace utils::helpers
#endif

#ifndef HELPERS_TPP
#define HELPERS_TPP

#include "utils/helpers/helpers.hpp"
#include "utils/ui/appearance.hpp"
#include <algorithm>
#include <cmath>
#include <queue>
#include <vector>

#include <sys/ioctl.h>
#include <unistd.h>

namespace utils::helpers
{

template <class A, class Ord, class Ext>
std::vector<std::vector<std::pair<double, A> > >
DelayDijkstra(const Graphs::Graph<A> &graph, int dest)
{
	Ord ord;
	Ext ext;
	const int V = graph.V;

	std::vector<std::vector<A> > opt(V);
	std::vector<std::vector<std::pair<double, A> > > opt_time(V);

	using Triple = std::tuple<double, int, A>; // (tau, node, attr)
	auto comp = [](const Triple &lhs, const Triple &rhs) {
		return std::get<0>(lhs) > std::get<0>(rhs);
	};

	auto in = [](const A &a, const std::vector<A> &set) {
		return std::find(set.begin(), set.end(), a) != set.end();
	};

	auto select = [&](std::vector<A> &set, const A &a) -> bool {
		for (const auto &b : set)
			if (ord(b, a)) {
				return false;
			}
		set.erase(std::remove_if(set.begin(), set.end(),
					 [&](const A &b) { return ord(a, b); }),
			  set.end());
		set.push_back(a);
		return true;
	};

	std::priority_queue<Triple, std::vector<Triple>, decltype(comp)> pq(
		comp);

	A neutral = Ord::Neutral();
	pq.push(std::make_tuple(0.0, dest, neutral));

	while (!pq.empty()) {
		auto [tau, u, a] = pq.top();
		pq.pop();
		if (!in(a, opt[u])) {
			if (select(opt[u], a)) {
				opt_time[u].push_back({ tau, a });
				for (auto &[x, edge] : graph.adjListIn[u]) {
					auto &[delay, active, link_attr] = edge;
					A candidate = ext(link_attr, a);
					pq.push(std::make_tuple(tau + delay, x,
								candidate));
				}
			}
		}
	}
	return opt_time;
}

template <class A, class Ord>
double
WorstDelay(const std::vector<std::vector<std::pair<double, A> > > &opt_time)
{
	Ord ord;
	double D = 0.0;
	for (const auto &pairs : opt_time) {
		if (pairs.size() < 2)
			continue;
		for (const auto &[ti, ai] : pairs) {
			for (const auto &[tj, aj] : pairs) {
				if (ai == aj)
					continue;
				if (ord(ai, aj)) {
					double gap = std::max(ti - tj, 0.0);
					D = std::max(D, gap);
				}
			}
		}
	}
	return D;
}

template <class A, class Ord, class Ext>

std::size_t Compute_N(const std::string &filename, double Delta)
{
	auto graph = Graphs::Graph<A>::readGraph(filename);
	double D = 0.0;

	std::vector<std::pair<int, double> > results;
	for (int u = 0; u < graph.V; ++u) {
		auto opt_time = DelayDijkstra<A, Ord, Ext>(graph, u);
		double delay = WorstDelay<A, Ord>(opt_time);
		D = std::max(D, delay);
		results.push_back({ u, delay });
	}

	printDelays(results, filename, D);

	const auto N = static_cast<std::size_t>(std::ceil(D / Delta));
	return N;
}

void printDelays(std::vector<std::pair<int, double> > &results,
		 const std::string &filename, double D)
{
	struct winsize w{};
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	if (w.ws_col % 2 != 0)
		w.ws_col -= 1;

	constexpr int COL_WIDTH = 22;
	const int n_cols = std::max(1, w.ws_col / COL_WIDTH);
	const int total = static_cast<int>(results.size());

	std::cout << ui::cyan("\nWorst delay per destination — " + filename +
			      " (" + std::to_string(total) + " destinations)" +
			      "\n\n");

	tabulate::Table t;

	tabulate::Table::Row_t header;
	for (int c = 0; c < n_cols; ++c) {
		header.push_back("Dest");
		header.push_back("D");
	}
	t.add_row(header);
	t[0].format()
		.font_style({ tabulate::FontStyle::bold })
		.font_color(tabulate::Color::cyan)
		.font_align(tabulate::FontAlign::center);

	int n_rows = static_cast<int>(
		std::ceil(static_cast<double>(total) / n_cols));

	std::vector<std::pair<int, int> > cells;

	for (int row = 0; row < n_rows; ++row) {
		tabulate::Table::Row_t r;
		for (int col = 0; col < n_cols; ++col) {
			int idx = col * n_rows + row;
			if (idx < total) {
				r.push_back(std::to_string(results[idx].first));
				std::ostringstream ss;
				ss << std::fixed << std::setprecision(2)
				   << results[idx].second;
				if (D == results[idx].second) {
					cells.push_back({ row, col });
				}
				r.push_back(ss.str());
			} else {
				r.push_back("");
				r.push_back("");
			}
		}
		t.add_row(r);
	}

	for (int c = 0; c < n_cols; ++c) {
		t.column(c * 2).format().font_color(tabulate::Color::cyan);
	}

	t[0].format()
		.font_style({ tabulate::FontStyle::bold })
		.font_color(tabulate::Color::yellow);

	for (auto &[fst, snd] : cells) {
		t[fst + 1][snd * 2 + 1]
			.format()
			.font_color(tabulate::Color::green)
			.font_style({ tabulate::FontStyle::bold });
	}

	std::cout << t << "\n";
}

}
#endif
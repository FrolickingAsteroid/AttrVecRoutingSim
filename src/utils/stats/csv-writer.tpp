#ifndef CSV_WRITER_TPP
#define CSV_WRITER_TPP

#include "utils/stats/csv-writer.hpp"

#include <ranges>

namespace utils::stats
{

template <class R>
void Recorder<R>::recordDest(int dest, const Data<R> &data, std::optional<std::pair<int, int>> failure)
{
	records.push_back({ dest, failure, data });
}

template <class R>
void Recorder<R>::writeConvergenceRow(std::ostream			&out,
				      int				 dest,
				      int				 u,
				      const Data<R>			&data,
				      std::optional<std::pair<int, int>> failure)
{
	auto it = data.finalValue.find(u);
	if (it == data.finalValue.end())
		return;
	if (failure)
		out << "(" << failure->first << ";" << failure->second << "),";
	out << dest << "," << u << "," << data.lastChangeTime[u] << "," << data.msgCount[u] << ","
	    << data.changeCounts[u] << "," << it->second << "\n";
}

template <class R> void Recorder<R>::writeTrafficRow(std::ostream &out, int dest, int msgs)
{
	out << dest << "," << msgs << "\n";
}

template <class R> void Recorder<R>::writeTrafficRow(std::ostream &out, std::pair<int, int> failure, int msgs)
{
	out << "(" << failure.first << ";" << failure.second << ")," << msgs << "\n";
}

template <class R> SummaryStats Recorder<R>::Summary(int V)
{
	SummaryStats stats{};

	for (const auto &[dest, failure, data] : records) {
		int dest_msgs = 0;

		for (int u = 0; u < V; u++) {
			if (u == dest)
				continue;
			stats.total_nodes++;

			if (data.changeCounts[u] != 0) {
				stats.term.add(data.lastChangeTime[u]);
			}

			stats.changes.add(static_cast<double>(data.changeCounts[u]));
			stats.nodes.add(static_cast<double>(data.msgCount[u]));

			dest_msgs += data.msgCount[u];
			writeConvergenceRow(convergence_out, dest, u, data, failure);
		}

		stats.total_msgs += dest_msgs;

		if (failure) {
			stats.msgs_per_link_failure[*failure].add(dest_msgs);
		} else {
			stats.msgs_per_dest[dest].add(dest_msgs);
		}
	}

	// finalize averages
	stats.term.finalize();
	stats.changes.finalize();
	stats.nodes.finalize();
	for (auto &e : stats.msgs_per_dest | std::views::values)
		e.finalize();
	for (auto &e : stats.msgs_per_link_failure | std::views::values)
		e.finalize();

	if (stats.msgs_per_link_failure.empty()) {
		for (const auto &[dest, entry] : stats.msgs_per_dest) {
			stats.msg.add(entry.total);
			writeTrafficRow(traffic_out, dest, entry.total);
		}
	} else {
		for (const auto &[failure, entry] : stats.msgs_per_link_failure) {
			stats.msg.add(entry.total);
			writeTrafficRow(traffic_out, failure, entry.total);
		}
	}
	stats.msg.count = stats.msgs_per_link_failure.empty() ?
				  static_cast<int>(stats.msgs_per_dest.size()) :
				  static_cast<int>(stats.msgs_per_link_failure.size());
	stats.msg.finalize();

	return stats;
}

}
#endif

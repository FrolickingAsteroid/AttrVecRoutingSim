#ifndef CSV_WRITER_TPP
#define CSV_WRITER_TPP

#include <ranges>

#include "utils/stats/csv-writer.hpp"

namespace utils::stats
{
template <class R>
void Recorder<R>::recordDest(int dest, const Data<R> &data, std::optional<std::pair<int, int>> failure)
{
	records.push_back({dest, failure, data});
}

template <class R>
void Recorder<R>::writeConvergenceRow(std::ostream &out, int dest, int u, const Data<R> &data,
				      std::optional<std::pair<int, int>> failure)
{
	auto it = data.finalValue.find(u);
	if (it == data.finalValue.end())
		return;
	if (failure)
		out << "(" << failure->first << ";" << failure->second << "),";
	out << dest << "," << u << "," << data.lastChangeTime[u] << "," << data.changeCounts[u] << "," << it->second
	    << "\n";
}

template <class R>
void Recorder<R>::writeMsgLogRow(std::ostream &out, std::optional<std::pair<int, int>> failure, int dest, int node,
				 double time, int msg)
{
	if (failure)
		out << "(" << failure->first << "-" << failure->second << "),";
	out << dest << "," << node << "," << time << "," << msg << "\n";
}

template <class R>
SummaryStats Recorder<R>::Summary(int V)
{
	SummaryStats stats{};

	for (const auto &[dest, failure, data] : records)
	{

		for (int u = 0; u < V; u++)
		{
			if (u == dest)
			{
				continue;
			}

			stats.total_nodes++;
			if (data.changeCounts[u] != 0)
			{
				stats.term.add(data.lastChangeTime[u]);
			}

			stats.changes.add(static_cast<double>(data.changeCounts[u]));
			stats.nodes.add(static_cast<double>(data.msgCount[u]));

			writeConvergenceRow(convergence_out, dest, u, data, failure);
		}

		for (const auto &[time, per_node] : data.msgLog)
		{
			for (const auto &[node, msg] : per_node)
			{
				writeMsgLogRow(msglog_out, failure, dest, node, time, msg);
			}
		}
	}

	// finalize averages
	stats.term.finalize();
	stats.changes.finalize();
	stats.nodes.finalize();

	return stats;
}
} // namespace utils::stats
#endif

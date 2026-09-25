#ifndef CSV_WRITER_TPP
#define CSV_WRITER_TPP

#include <ranges>

#include "utils/stats/csv-writer.hpp"

namespace utils::stats
{

template <class R>
template <class A>
void Recorder<R>::recordProgress(int dest, int node, double time, A &best, A &wind, std::string tag)
{
	progress_out << dest << "," << node << "," << time << "," << best << "," << wind << "," << tag << "\n";
}

template <class R>
void Recorder<R>::recordDest(int dest, const Data<R> &data, std::optional<std::pair<int, int>> failure)
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

template <class R>
void Recorder<R>::recordDest(int dest, const Data<R> &data, const std::vector<std::pair<int, int>> &cutset)
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

		writeConvergenceRow(convergence_out, dest, u, data, cutset);
	}

	for (const auto &[time, per_node] : data.msgLog)
	{
		for (const auto &[node, msg] : per_node)
		{
			writeMsgLogRow(msglog_out, cutset, dest, node, time, msg);
		}
	}
}

template <class R>
template <class A>
void Recorder<R>::recordWind(int dest, const std::vector<std::vector<std::pair<double, A>>> &opt_time,
			     std::vector<int> &wind_size, std::optional<std::pair<int, int>> failure)
{
	for (std::size_t u = 0; u < opt_time.size(); ++u)
	{
		writeWind(wind_out, dest, static_cast<int>(u), opt_time[u], wind_size[u], failure);
	}
}

template <class R>
template <class A>
void Recorder<R>::writeWind(std::ostream &out, int dest, int u, const std::vector<std::pair<double, A>> &opt_time_u,
			    int wind_size, const std::optional<std::pair<int, int>> &failure)
{
	std::ostringstream s;
	bool               first = true;
	for (const auto &[delay, attr] : opt_time_u)
	{
		if (!first)
			s << "; ";
		s << delay << ": " << attr;
		first = false;
	}

	if (failure)
	{
		out << "(" << failure->first << ";" << failure->second << "),";
	}
	out << dest << "," << u << "," << s.str() << "," << wind_size << "\n";
}

template <class R>
void Recorder<R>::writeConvergenceRow(std::ostream &out, int dest, int u, const Data<R> &data,
				      const std::optional<std::pair<int, int>> &failure)
{
	auto it = data.finalValue.find(u);
	if (it == data.finalValue.end())
	{
		return;
	}
	if (failure)
	{
		if (failure->second == -1)
		{
			out << failure->first << ",";
		}
		else
		{
			out << "(" << failure->first << ";" << failure->second << "),";
		}
	}
	out << dest << "," << u << "," << data.lastChangeTime[u] << "," << data.changeCounts[u] << "," << it->second
	    << "\n";
}

template <class R>
void Recorder<R>::writeConvergenceRow(std::ostream &out, int dest, int u, const Data<R> &data,
				      const std::vector<std::pair<int, int>> &cutset)
{
	auto it = data.finalValue.find(u);
	if (it == data.finalValue.end())
	{
		return;
	}

	for (const auto &[a, b] : cutset)
	{
		out << "(" << a << ";" << b << ")";
	}
	if (!cutset.empty())
	{
		out << ",";
	}

	out << dest << "," << u << "," << data.lastChangeTime[u] << "," << data.changeCounts[u] << "," << it->second
	    << "\n";
}

template <class R>
void Recorder<R>::writeMsgLogRow(std::ostream &out, std::optional<std::pair<int, int>> failure, int dest, int node,
				 double time, int msg)
{
	if (failure)
	{
		if (failure->second == -1)
		{
			out << failure->first << ",";
		}
		else
		{
			out << "(" << failure->first << ";" << failure->second << "),";
		}
	}
	out << dest << "," << node << "," << time << "," << msg << "\n";
}

template <class R>
void Recorder<R>::writeMsgLogRow(std::ostream &out, const std::vector<std::pair<int, int>> &cutset, int dest, int node,
				 double time, int msg)
{
	for (const auto &[a, b] : cutset)
	{
		out << "(" << a << "-" << b << ")";
	}
	if (!cutset.empty())
	{
		out << ",";
	}
	out << dest << "," << node << "," << time << "," << msg << "\n";
}

template <class R>
SummaryStats Recorder<R>::Summary()
{
	// finalize averages
	stats.term.finalize();
	stats.changes.finalize();
	stats.nodes.finalize();

	return stats;
}
} // namespace utils::stats
#endif

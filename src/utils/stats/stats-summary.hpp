#ifndef STATS_SUMMARY_HPP
#define STATS_SUMMARY_HPP

#include <algorithm>

#include "utils/stats/data.hpp"

namespace utils::stats
{

// ─── per-destination summary ──────────────────────────────────────────────────
// Computes and prints avg/min/max statistics for a single destination run.
// Call once per destination after proto.Run(), passing proto.data and
// the destination node id so the output is labelled correctly.
//
// Cycle stats are only printed if data.cycle is populated.

template <class R> SummaryStats Summary(const std::vector<Record<R>> &records)
{
	SummaryStats stats{};

	for (const auto &[dest, data] : records) {
		const int n = static_cast<int>(data.lastChangeTime.size());
		stats.total_nodes += n;

		for (int u = 0; u < n; ++u) {
			if (u == dest) {
				continue;
			}
			stats.avg_term += data.lastChangeTime[u];
			stats.min_term = std::min(stats.min_term, data.lastChangeTime[u]);
			stats.max_term = std::max(stats.max_term, data.lastChangeTime[u]);

			stats.avg_changes += data.changeCounts[u];
			stats.min_changes =
				std::min(stats.min_changes, (double)data.changeCounts[u]);
			stats.max_changes =
				std::max(stats.max_changes, (double)data.changeCounts[u]);

			stats.avg_msg += data.msgCount[u];
			stats.min_msg = std::min(stats.min_msg, (double)data.msgCount[u]);
			stats.max_msg = std::max(stats.max_msg, (double)data.msgCount[u]);
		}
	}

	stats.avg_term /= stats.total_nodes;
	stats.avg_changes /= stats.total_nodes;
	stats.avg_msg /= stats.total_nodes;

	return stats;
}
}

#endif
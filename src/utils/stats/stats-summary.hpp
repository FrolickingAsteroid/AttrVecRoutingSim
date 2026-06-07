#ifndef STATS_SUMMARY_HPP
#define STATS_SUMMARY_HPP

#include "utils/stats/data.hpp"

#include <algorithm>
#include <ostream>

namespace utils::stats
{

// ─── per-destination summary ──────────────────────────────────────────────────
// Computes and prints avg/min/max statistics for a single destination run.
// Call once per destination after proto.Run(), passing proto.data and
// the destination node id so the output is labelled correctly.
//
// Cycle stats are only printed if data.cycle is populated.

template <class R> void PrintDestSummary(const Data<R> &data, int dest, std::ostream &out)
{
	int n = static_cast<int>(data.lastChangeTime.size());

	double avg_term = 0, min_term = 1e18, max_term = 0;
	double avg_changes = 0, min_changes = 1e18, max_changes = 0;
	double avg_msg = 0, min_msg = 1e18, max_msg = 0;

	for (int u = 0; u < n; ++u) {
		avg_term += data.lastChangeTime[u];
		min_term = std::min(min_term, data.lastChangeTime[u]);
		max_term = std::max(max_term, data.lastChangeTime[u]);

		avg_changes += data.changeCounts[u];
		min_changes = std::min(min_changes, (double)data.changeCounts[u]);
		max_changes = std::max(max_changes, (double)data.changeCounts[u]);

		avg_msg += data.msgCount[u];
		min_msg = std::min(min_msg, (double)data.msgCount[u]);
		max_msg = std::max(max_msg, (double)data.msgCount[u]);
	}

	avg_term /= n;
	avg_changes /= n;
	avg_msg /= n;

	out << "\n─── Destination " << dest << " (" << n
	    << " nodes) ────────────────────────────\n\n";

	out << "  Termination time    avg: " << avg_term << "  min: " << min_term
	    << "  max: " << max_term << " ms\n";

	out << "  Attribute changes   avg: " << avg_changes << "  min: " << min_changes
	    << "  max: " << max_changes << "\n";

	out << "  Messages sent       avg: " << avg_msg << "  min: " << min_msg
	    << "  max: " << max_msg << "\n";
	out << "\n";
}

template <class R> struct DestRecord {
	int	 dest;
	Data<R> &data;
};

template <class R>
void PrintAggregateSummary(const std::vector<DestRecord<R>> &records, std::ostream &out)
{
	if (records.empty())
		return;

	double avg_term = 0, min_term = 1e18, max_term = 0;
	double avg_changes = 0, min_changes = 1e18, max_changes = 0;
	double avg_msg = 0, min_msg = 1e18, max_msg = 0;
	double avg_ct = 0, min_ct = 1e18, max_ct = 0;
	double avg_cc = 0, min_cc = 1e18, max_cc = 0;
	int    ct_n = 0, cc_n = 0;
	int    total_nodes = 0;

	for (const auto &[dest, data] : records) {
		int n = static_cast<int>(data.lastChangeTime.size());
		total_nodes += n;

		for (int u = 0; u < n; ++u) {
			avg_term += data.lastChangeTime[u];
			min_term = std::min(min_term, data.lastChangeTime[u]);
			max_term = std::max(max_term, data.lastChangeTime[u]);

			avg_changes += data.changeCounts[u];
			min_changes = std::min(min_changes, (double)data.changeCounts[u]);
			max_changes = std::max(max_changes, (double)data.changeCounts[u]);

			avg_msg += data.msgCount[u];
			min_msg = std::min(min_msg, (double)data.msgCount[u]);
			max_msg = std::max(max_msg, (double)data.msgCount[u]);
		}
	}

	avg_term /= total_nodes;
	avg_changes /= total_nodes;
	avg_msg /= total_nodes;
	if (ct_n > 0)
		avg_ct /= ct_n;
	if (cc_n > 0)
		avg_cc /= cc_n;

	out << "\n═══ Aggregate summary (" << records.size() << " destinations, " << total_nodes
	    << " node-runs) "
	    << "═══════════════════════\n\n";

	out << "  Termination time    avg: " << avg_term << "  min: " << min_term
	    << "  max: " << max_term << " ms\n";

	out << "  Attribute changes   avg: " << avg_changes << "  min: " << min_changes
	    << "  max: " << max_changes << "\n";

	out << "  Messages sent       avg: " << avg_msg << "  min: " << min_msg
	    << "  max: " << max_msg << "\n";
	out << "\n";
}

} // namespace utils::stats

#endif
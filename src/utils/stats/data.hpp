#ifndef DATA_HPP
#define DATA_HPP

#include <optional>
#include <string>
#include <vector>

namespace utils::stats
{

// ─── Data<R> ─────────────────────────────────────────────────────────────────
// Per-destination data collected during one simulation run.
//
// R is the elected attribute type (e.g. BGPAttribute<A>).
// Only the endpoint matters for convergence analysis:
//   - lastChangeTime[u]  — time of u's last attribute change (termination time)
//   - changeCounts[u]    — total number of attribute changes at u
//   - finalValue[u]      — elected attribute at convergence
//   - messageCount       — total messages sent during the run

template <class R> struct Data {
	explicit Data(int V)
		: lastChangeTime(V, 0.0)
		, changeCounts(V, 0)
		, msgCount(V)
	{
	}

	// Called by the protocol every time node u changes its elected attribute.
	void recordChange(const int node, const double time, const R &value)
	{
		lastChangeTime[node] = time;
		++changeCounts[node];
		finalValue[node] = value;
	}

	// Called by the protocol every time a message is sent.
	void recordMessage(const int node)
	{
		++msgCount[node];
	}

	std::vector<double>	   lastChangeTime;
	std::vector<int>	   changeCounts;
	std::unordered_map<int, R> finalValue;
	std::vector<int>	   msgCount;
};

}

#endif
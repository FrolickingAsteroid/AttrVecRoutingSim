#ifndef DATA_HPP
#define DATA_HPP

#include <unordered_map>
#include <vector>

namespace utils::stats
{
template <class R>
struct Data
{
	explicit Data(const std::size_t V)
	    : msgCount(V)
	    , changeCounts(V, 0)
	    , lastChangeTime(V, 0.0)
	{
	}

	void setOffset(double t)
	{
		offset = t;
	}

	void recordChange(const int node, const double time, const R &value)
	{
		finalValue[node] = value;

		if (log == false)
			return;

		lastChangeTime[node] = time - offset;
		++changeCounts[node];
	}

	void recordMessage(const int node, const double time)
	{
		++msgCount[node];
		++msgLog[time][node];
	}

	double offset = 0.0;
	bool   log    = false;

	std::vector<int>                     msgCount;
	std::vector<int>                     changeCounts;
	std::vector<double>                  lastChangeTime;
	std::unordered_map<int, R>           finalValue;
	std::map<double, std::map<int, int>> msgLog;
};
} // namespace utils::stats

#endif

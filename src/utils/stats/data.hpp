#ifndef DATA_HPP
#define DATA_HPP

#include <iostream>
#include <unordered_map>
#include <functional>
#include <vector>

namespace utils::stats
{

template <class R> struct Data {
	explicit Data(const int V)
		: lastChangeTime(V, 0.0)
		, changeCounts(V, 0)
		, msgCount(V)
		, Conv(V, false)
	{
	}

	void setOffset(double t)
	{
		offset = t;
	}

	std::function<bool(int)> hasConverged = nullptr;

	// Called by the protocol every time node u changes its elected attribute.
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

	double				     offset = 0.0;
	bool				     log    = false;
	std::vector<double>		     lastChangeTime;
	std::vector<int>		     changeCounts;
	std::unordered_map<int, R>	     finalValue;
	std::map<double, std::map<int, int>> msgLog;
	std::vector<int>		     msgCount;
	std::vector<bool>		     Conv;
};

}

#endif
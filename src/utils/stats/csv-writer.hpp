#ifndef CSV_RECORDER_HPP
#define CSV_RECORDER_HPP

#include "utils/stats/csv-output.hpp"
#include "utils/stats/data.hpp"

#include <filesystem>
#include <fstream>
#include <map>
#include <string>

namespace utils::stats
{

template <class R> struct Record {
	int	dest;
	Data<R> data;
};

/**
 * Implementation of a simple csv management class
 *
 * Owns both output files for the lifetime of the simulation run.
 * Files are opened once in the constructor and closed automatically
 * when the recorder is destroyed.
 *
 * @tparam R Routing message metric to be recorded
 */
template <class R> class Recorder {
    public:
	Recorder(const std::filesystem::path		  &input,
		 const std::string			  &proto,
		 const std::map<std::string, std::string> &map)
		: records()
	{
		convergence_out = openFile(makeOutputPath(input, proto, map, "convergence"));
		convergence_out << convergenceHeader();

		traffic_out = openFile(makeOutputPath(input, proto, map, "traffic"));
		traffic_out << trafficHeader();
	}

	void recordDest(int dest, const Data<R> &data)
	{
		records.push_back({ dest, data });
		int n = static_cast<int>(data.lastChangeTime.size());
		for (int u = 0; u < n; ++u) {
			if (u == dest) {
				continue;
			}
			writeConvergenceRow(convergence_out, dest, u, data);
			writeTrafficRow(traffic_out, dest, u, data);
		}
	}

	std::vector<Record<R>> getRecords()
	{
		return records;
	}

	static std::string convergenceHeader()
	{
		return "dest,node,stab_time,change_count,metric\n";
	}

	static void writeConvergenceRow(std::ostream &out, int dest, int u, const Data<R> &data)
	{
		out << dest << "," << u << "," << data.lastChangeTime[u] << ","
		    << data.changeCounts[u] << "," << data.finalValue.at(u) << "\n";
	}

	static std::string trafficHeader()
	{
		return "dest,node,msg_count\n";
	}

	static void writeTrafficRow(std::ostream &out, int dest, int u, const Data<R> &data)
	{
		out << dest << "," << u << "," << data.msgCount[u] << "\n";
	}

    private:
	std::vector<Record<R>> records;
	std::ofstream	       convergence_out;
	std::ofstream	       traffic_out;
};

}

#endif
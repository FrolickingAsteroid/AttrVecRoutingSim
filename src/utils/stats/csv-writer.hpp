#ifndef CSV_WRITER_HPP
#define CSV_WRITER_HPP

#include <filesystem>
#include <fstream>
#include <map>
#include <string>

#include "utils/stats/csv-output.hpp"
#include "utils/stats/data.hpp"

namespace utils::stats
{

struct MsgStats
{
	int    count = 0;
	double total = 0;
	double avg   = 0.0;
	double min   = INT_MAX;
	double max   = 0.0;

	void add(const double msgs)
	{
		total += msgs;
		count += 1;
		min = std::min(min, msgs);
		max = std::max(max, msgs);
	}

	void finalize()
	{
		avg = count > 0 ? static_cast<double>(total) / count : 0.0;
	}
};

struct SummaryStats
{
	// per-node
	MsgStats term;    // per node termination time
	MsgStats changes; // per node attribute changes
	MsgStats nodes;   // per node message count
	MsgStats msg;     // per dest or per failure message count

	int total_nodes = 0;
	int total_msgs  = 0;

	// per-dest or per-failure
	std::map<int, MsgStats>                 msgs_per_dest;
	std::map<std::pair<int, int>, MsgStats> msgs_per_link_failure;
};

template <class R> struct Record
{
	int                                dest{};
	std::optional<std::pair<int, int>> failure;
	Data<R>                            data;
};

/**
 * Implementation of a simple csv management class
 *
 * Owns both output files for the lifetime of the simulation run.
 * Files are opened once in the constructor and closed automatically
 * when the recorder is destroyed.
 *
 * @tparam R Routing metric to be recorded
 */
template <class R> class Recorder
{
    public:
	Recorder(const std::filesystem::path &input, const std::string &proto,
		 const std::map<std::string, std::string> &map)
	    : records()
	{
		bool append       = (map.at("append") == "1");
		bool link_failure = (map.at("S_") == "linkFailure");

		convergence_out =
		    openFile(makeOutputPath(input, proto, map, "convergence"), append, convergenceHeader(link_failure));

		traffic_out =
		    openFile(makeOutputPath(input, proto, map, "traffic"), append, trafficHeader(link_failure));

		msglog_out = openFile(makeOutputPath(input, proto, map, "msglog"), append, msgLogHeader(link_failure));
	}

	std::vector<Record<R>> getRecords()
	{
		return records;
	}

	static std::string convergenceHeader(bool with_failure = false)
	{
		return (with_failure ? "failure," : "") +
		       std::string("dest,node,stab_time,msg_count,change_count,metric\n");
	}

	static std::string msgLogHeader(bool with_failure = false)
	{
		return (with_failure ? "failure," : "") + std::string("dest,node,time,msg\n");
	}

	static std::string trafficHeader(bool with_failure = false)
	{
		return (with_failure ? "failure," : "dest,") + std::string("total_msgs\n");
	}

	void recordDest(int dest, const Data<R> &data, std::optional<std::pair<int, int>> failure = std::nullopt);

	static void writeTrafficRow(std::ostream &out, std::pair<int, int> failure, int msgs);
	static void writeTrafficRow(std::ostream &out, int dest, int msgs);

	static void writeMsgLogRow(std::ostream &out, std::optional<std::pair<int, int>> failure, int dest, int node,
				   double time, int msg);

	static void writeConvergenceRow(std::ostream &out, int dest, int u, const Data<R> &data,
					std::optional<std::pair<int, int>> failure);

	SummaryStats Summary(int V);

    private:
	std::vector<Record<R>> records;
	std::ofstream          convergence_out;
	std::ofstream          traffic_out;
	std::ofstream          msglog_out;
};

} // namespace utils::stats

#include "utils/stats/csv-writer.tpp"

#endif
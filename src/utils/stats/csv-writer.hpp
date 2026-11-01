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
		avg = count > 0 ? total / count : 0.0;
	}
};

struct SummaryStats
{
	// per-node
	MsgStats term;    // per node termination time
	MsgStats changes; // per node attribute changes
	MsgStats nodes;   // per node message count

	int total_nodes = 0;
	int total_msgs  = 0;
};

/**
 * Implementation of a simple csv management class
 *
 * Owns both output files for the lifetime of the simulation run.
 * Files are opened once in the constructor and closed automatically
 * when the recorder is destroyed.
 *
 * @tparam R message to be recorded
 */
template <class R>
class Recorder
{
	int          V = 0;
	SummaryStats stats{};

    public:
	Recorder(const std::filesystem::path &input, const std::string &proto,
		 const std::map<std::string, std::string> &map, const int V_)
	    : V(V_)
	{
		auto scenario     = map.at("S_");
		bool append       = (map.at("append") == "1");
		bool link_failure = (scenario == "linkFailure" || scenario == "cutset");

		if (proto == "beaconing")
		{
			progress_out =
			    openFile(makeOutputPath(input, proto, map, "progress", scenario), append, progressHeader());
		}

		if (proto == "window")
		{
			wind_out = openFile(makeOutputPath(input, proto, map, "", scenario), append, windHeader(link_failure));
		}
		else
		{
			convergence_out = openFile(makeOutputPath(input, proto, map, "convergence",scenario), append,
						   convergenceHeader(link_failure));

			msglog_out =
			    openFile(makeOutputPath(input, proto, map, "msglog",scenario), append, msgLogHeader(link_failure));
		}

		if (proto == "bgp" || proto == "rcn")
		{
			state_out =
			    openFile(makeOutputPath(input, proto, map, "state", scenario), append, stateHeader(link_failure));
		}
	}

	static std::string progressHeader()
	{
		return "dest,node,time,opt,wind,tag\n";
	}

	static std::string windHeader(bool with_failure = false)
	{
		return (with_failure ? "failure," : "") + std::string("dest,node,opt_time,wind_size\n");
	}

	static std::string convergenceHeader(bool with_failure = false)
	{
		return (with_failure ? "failure," : "") + std::string("dest,node,stab_time,change_count,metric\n");
	}

	static std::string msgLogHeader(bool with_failure = false)
	{
		return (with_failure ? "failure," : "") + std::string("dest,node,time,msg\n");
	}

	static std::string stateHeader(bool with_failure = false)
	{
		return (with_failure ? "failure," : "") + std::string("dest,node,rib_size,seq_size\n");
	}

	void recordDest(int dest, const Data<R> &data, std::optional<std::pair<int, int>> failure = std::nullopt);
	void recordDest(int dest, const Data<R> &data, const std::vector<std::pair<int, int>> &cutset);

	template <class A>
	void recordWind(int dest, const std::vector<std::vector<std::pair<double, A>>> &opt_time,
			std::vector<long long> &wind_size, std::optional<std::pair<int, int>> failure = std::nullopt);

	template <class A>
	void recordProgress(int dest, int node, double time, A &best, A &wind, std::string tag);

	template <class A>
	static void writeWind(std::ostream &out, int dest, int u, const std::vector<std::pair<double, A>> &opt_time_u,
			      long long wind_size, const std::optional<std::pair<int, int>> &failure);

	static void writeMsgLogRow(std::ostream &out, std::optional<std::pair<int, int>> failure, int dest, int node,
				   double time, int msg);

	static void writeMsgLogRow(std::ostream &out, const std::vector<std::pair<int, int>> &cutset, int dest,
				   int node, double time, int msg);

	static void writeConvergenceRow(std::ostream &out, int dest, int u, const Data<R> &data,
					const std::optional<std::pair<int, int>> &failure);

	static void writeConvergenceRow(std::ostream &out, int dest, int u, const Data<R> &data,
					const std::vector<std::pair<int, int>> &cutset);

	static void writeStateRow(std::ostream &out, int dest, int u, const Data<R> &data,
				  const std::optional<std::pair<int, int>> &failure);

	static void writeStateRow(std::ostream &out, int dest, int u, const Data<R> &data,
				  const std::vector<std::pair<int, int>> &cutset);

	SummaryStats Summary();

    private:
	std::ofstream convergence_out;
	std::ofstream progress_out;
	std::ofstream msglog_out;
	std::ofstream wind_out;
	std::ofstream state_out;
};
} // namespace utils::stats

#include "utils/stats/csv-writer.tpp"

#endif
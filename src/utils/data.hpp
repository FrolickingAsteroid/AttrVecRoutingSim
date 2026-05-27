#ifndef DATA_HPP
#define DATA_HPP

#include <filesystem>
#include <vector>
#include <optional>

#include "utils/helpers.hpp"

namespace utils
{
enum class RegisterType { Set, NodeStab };

struct MsgEntry {
	double time;
	int node;

	std::string kind; // "announcement" or "propagate"
	uint64_t msg_count;
};

template <class R> struct Entry {
	double time;
	int node;
	R value;

	std::optional<std::vector<R> > set;
};

template <class R> struct Data {
	std::vector<Entry<R> > entries;
	std::vector<MsgEntry> cnt;

	void record(double time, int node, const R &value)
	{
		entries.push_back({ time, node, value, std::nullopt });
	}

	void record(double time, int node, const R &value,
		    const std::vector<R> &values)
	{
		entries.push_back({ time, node, value, values });
	}

	void record(double time, int node, const std::string &kind,
		    uint64_t count)
	{
		cnt.push_back({ time, node, kind, count });
	}
};

template <class R> class Registrar {
    public:
	Registrar(const std::filesystem::path &input_, int beacons_,
		  int destination_, double interval_, RegisterType type_,
		  const std::string &proto_)
		: input(input_)
		, beacons(beacons_)
		, destination(destination_)
		, interval(interval_)
		, type(type_)
		, proto(proto_)
	{
		if (type == RegisterType::NodeStab && destination < 0) {
			all_stab = makeFile("all", "stab");
			all_stab << "dest,node,stab_time,metric\n";
		}
		all_overhead = makeFile("all", "msgCount");
		all_overhead << "dest,time,node,kind,msg_count\n";
	}

	void RegisterDestination(const int dest, const Data<R> &data)
	{
		if (type == RegisterType::Set) {
			RegisterSet(dest, data);
		} else if (type == RegisterType::NodeStab) {
			RegisterNodeStab(dest, data);
			RegisterMsgCnt(dest, data);
		}
	}

    private:
	std::filesystem::path input;
	int beacons;
	int destination;
	double interval;
	RegisterType type;
	std::ofstream all_stab;
	std::ofstream all_overhead;
	std::string proto;

	std::filesystem::path makeOutputPath(const std::string &dest,
					     const std::string &suffix) const
	{
		auto p = input;
		auto stem = p.stem();
		auto dataset_id = p.parent_path().filename();
		auto dataset_fam = p.parent_path().parent_path().filename();
		auto output_dir = std::filesystem::path("data") / "output" /
				  dataset_fam / dataset_id / proto;

		std::filesystem::create_directories(output_dir);

		std::string name =
			stem.string() + "_" +
			(dest == "all" ? "all" : "d" + dest) + "_p" +
			std::to_string(interval).substr(
				0, std::to_string(interval).find('.') + 2) +
			"_rc" + std::to_string(beacons) + "_" + suffix + ".csv";

		return output_dir / name;
	}

	std::ofstream makeFile(const std::string &dest,
			       const std::string &suffix) const
	{
		const std::filesystem::path file = makeOutputPath(dest, suffix);
		auto out = openFile(file);
		return out;
	}

	static std::ofstream openFile(const std::filesystem::path &f)
	{
		std::ofstream out(f);
		if (!out.is_open())
			throw std::runtime_error("Could not open file: " +
						 f.string());
		return out;
	}

	void RegisterSet(int dest, const Data<R> &data) const;

	void RegisterMsgCnt(int dest, const Data<R> &data);

	void RegisterNodeStab(int dest, const Data<R> &data);

	void WriteStab(int dest, const Data<R> &data, std::ostream &out);

	void WriteMsgCnt(int dest, const Data<R> &data, std::ostream &out);
};

} // namespace utils

#include "utils/data.tpp"
#endif
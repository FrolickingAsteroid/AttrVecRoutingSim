// utils/data.hpp
#ifndef DATA_HPP
#define DATA_HPP

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

namespace utils
{
template <class T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v)
{
	os << "[";
	for (std::size_t i = 0; i < v.size(); ++i) {
		if (i > 0)
			os << " ";
		os << v[i];
	}
	os << "]";
	return os;
}

enum class RegisterType { Set, NodeStab };

template <class R> struct Entry {
	double time;
	int node;
	R value;

	std::optional<std::vector<R> > set;
};

template <class R> struct Data {
	std::vector<Entry<R> > entries;

	void record(double time, int node, const R &value)
	{
		entries.push_back({ time, node, value, std::nullopt });
	}

	void record(double time, int node, const R &value,
		    const std::vector<R> &values)
	{
		entries.push_back({ time, node, value, values });
	}
};

template <class R> class Registrar {
    public:
	Registrar(const std::filesystem::path &input_, int beacons_,
		  int destination_, double interval_, RegisterType type_)
		: input(input_)
		, beacons(beacons_)
		, destination(destination_)
		, interval(interval_)
		, type(type_)
	{
		if (type == RegisterType::NodeStab && destination < 0) {
			auto f = makeOutputPath("all", "stab");
			all_stab.open(f);
			if (!all_stab.is_open())
				throw std::runtime_error(
					"Could not open file: " + f.string());
			all_stab << "dest,node,stab_time\n";
		}
	}

	void RegisterDestination(int dest, const Data<R> &data)
	{
		if (type == RegisterType::Set) {
			RegisterSet(dest, data);
		} else if (type == RegisterType::NodeStab) {
			RegisterNodeStab(dest, data);
		}
	}

    private:
	std::filesystem::path input;
	int beacons;
	int destination;
	double interval;
	RegisterType type;
	std::ofstream all_stab;

	std::filesystem::path makeOutputPath(const std::string &dest,
					     const std::string &suffix) const
	{
		auto p = input;
		auto stem = p.stem();
		auto dataset_id = p.parent_path().filename();
		auto dataset_fam = p.parent_path().parent_path().filename();
		auto output_dir = std::filesystem::path("data") / "output" /
				  dataset_fam / dataset_id;

		std::filesystem::create_directories(output_dir);

		std::string name =
			stem.string() + "_" +
			(dest == "all" ? "all" : "d" + dest) + "_p" +
			std::to_string(interval).substr(
				0, std::to_string(interval).find('.') + 2) +
			"_rc" + std::to_string(beacons) + "_" + suffix + ".csv";

		return output_dir / name;
	}

	void RegisterSet(int dest, const Data<R> &data) const
	{
		auto out =
			openFile(makeOutputPath(std::to_string(dest), "set"));
		out << "time,node,value,set\n";
		for (auto &e : data.entries) {
			out << e.time << "," << e.node << ","
			    << "\"" << e.value << "\",";
			if (e.set)
				out << "\"" << *e.set << "\"";
			out << "\n";
		}
	}

	void WriteNodeStabRows(int dest, const Data<R> &data,
			       std::ostream &out) const
	{
		std::unordered_map<int, double> last_change;
		for (auto &e : data.entries) {
			if (e.set == std::nullopt) {
				last_change[e.node] = e.time;
			}
		}
		for (auto &[node, time] : last_change) {
			out << dest << "," << node << "," << time << "\n";
		}
	}

	void RegisterNodeStab(int dest, const Data<R> &data)
	{
		if (destination < 0) {
			WriteNodeStabRows(dest, data, all_stab);
			return;
		}
		auto out =
			openFile(makeOutputPath(std::to_string(dest), "stab"));
		out << "dest,node,stab_time\n";
		WriteNodeStabRows(dest, data, out);
	}

	static std::ofstream openFile(const std::filesystem::path &f)
	{
		std::ofstream out(f);
		if (!out.is_open())
			throw std::runtime_error("Could not open file: " +
						 f.string());
		return out;
	}
};

} // namespace utils
#endif
#ifndef DATA_HPP
#define DATA_HPP

#include <filesystem>
#include <vector>
#include <optional>

#include "utils/helpers.hpp"

namespace utils
{
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
			all_stab = makeFile("all", "stab");
			all_stab << "dest,node,stab_time\n";
		}
	}

	void RegisterDestination(const int dest, const Data<R> &data)
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

	void WriteStab(int dest, const Data<R> &data, std::ostream &out) const;

	void RegisterNodeStab(int dest, const Data<R> &data);
};

} // namespace utils

#include "utils/data.tpp"
#endif
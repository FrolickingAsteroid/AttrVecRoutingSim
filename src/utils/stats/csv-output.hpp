#ifndef CSV_OUTPUT_HPP
#define CSV_OUTPUT_HPP

#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

namespace utils
{

inline std::filesystem::path makeOutputPath(const std::filesystem::path &input, const std::string &proto,
					    const std::map<std::string, std::string> &params, const std::string &suffix)
{
	auto stem        = input.stem().string();
	auto dataset_id  = input.parent_path().filename().string();
	auto dataset_fam = input.parent_path().parent_path().filename().string();

	auto output_dir = std::filesystem::path("data") / "output" / dataset_fam / dataset_id / proto;

	std::filesystem::create_directories(output_dir);

	std::string param_str;
	for (const auto &[k, v] : params)
	{
		if (k == "append")
			continue;

		if (k == "notif")
		{
			if (v == "1")
			{
				param_str += "_";
				param_str += k;
			}
			continue;
		}

		if (!param_str.empty())
			param_str += "_";
		param_str += k + v;
	}

	std::string name = stem + (param_str.empty() ? "" : "_" + param_str) + "_" + suffix + ".csv";

	return output_dir / name;
}

// Opens a file for writing.
inline std::ofstream openFile(const std::filesystem::path &path, bool append, const std::string &header)
{
	bool write_header = !append || !std::filesystem::exists(path) || std::filesystem::file_size(path) == 0;

	auto mode = append ? (std::ios::out | std::ios::app) : std::ios::out;

	std::ofstream out(path, mode);
	if (!out.is_open())
		throw std::runtime_error("Could not open file: " + path.string());

	if (write_header)
		out << header;
	return out;
}
} // namespace utils

#endif

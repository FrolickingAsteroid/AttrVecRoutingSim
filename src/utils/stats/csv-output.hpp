#ifndef CSV_WRITER_HPP
#define CSV_WRITER_HPP

#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

namespace utils
{

// Constructs an output path of the form:
//
//   data/output/<proto>/<dataset_fam>/<dataset_id>/<stem>_<proto>_<k1><v1>_..._<suffix>.csv
//
// Topology is encoded entirely by the folder structure inherited from input.
// Run parameters are passed as a map so each protocol can include exactly
// what it needs (algebra type, N, dprop, delta, etc.) without this function
// knowing about any of them.
//
// params entries are appended in iteration order (std::map is sorted by key),
// so use keys that sort in the order you want them to appear in the filename.
inline std::filesystem::path makeOutputPath(const std::filesystem::path		     &input,
					    const std::string			     &proto,
					    const std::map<std::string, std::string> &params,
					    const std::string			     &suffix)
{
	auto stem	 = input.stem().string();
	auto dataset_id	 = input.parent_path().filename().string();
	auto dataset_fam = input.parent_path().parent_path().filename().string();

	auto output_dir =
		std::filesystem::path("data") / "output" / dataset_fam / dataset_id / proto;

	std::filesystem::create_directories(output_dir);

	std::string param_str;
	for (const auto &[k, v] : params) {
		if (!param_str.empty())
			param_str += "_";
		param_str += k + v;
	}

	std::string name =
		stem + (param_str.empty() ? "" : "_" + param_str) + "_" + suffix + ".csv";

	return output_dir / name;
}

// Opens a file for writing.
inline std::ofstream openFile(const std::filesystem::path &path)
{
	std::ofstream out(path);
	if (!out.is_open())
		throw std::runtime_error("Could not open file: " + path.string());
	return out;
}

} // namespace utils

#endif
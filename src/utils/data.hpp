#ifndef DATA_HPP
#define DATA_HPP

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>


#include "protocol/state/path-segment.hpp"
#include "utils/logger.hpp"

using namespace Protocol::State;

namespace utils {
    template<class A>
    struct DataEntry {
        double time;
        int node;

        Path<A> opt_attr;
        std::vector<Path<A> > set_seq_attr;
    };

    template<class A>
    struct Data {
        std::vector<DataEntry<A> > entries;

        static std::filesystem::path MakeOutputPath(const std::filesystem::path &input,
                                                    int beacons,
                                                    int dest) {
            std::filesystem::path p = input;
            std::filesystem::path filename_stem = p.stem();
            std::filesystem::path dataset_id = p.parent_path().filename();
            std::filesystem::path dataset_family = p.parent_path().parent_path().filename();
            std::filesystem::path output_dir =
                    std::filesystem::path("data") / "output" / dataset_family / dataset_id;

            std::filesystem::create_directories(output_dir);

            std::string output_name =
                    filename_stem.string()
                    + "-"
                    + std::to_string(beacons)
                    + "-"
                    + std::to_string(dest)
                    + ".csv";

            return output_dir / output_name;
        }

        void record(double time, int node, const Path<A> &opt_attr, std::vector<Path<A> > set_seq_attr) {
            entries.push_back(DataEntry<A>{time, node, opt_attr, set_seq_attr});
        }

        std::string attr_to_string(const Path<A> &p) const {
            std::ostringstream ss;

            ss << "("
                    << p.attribute.n
                    << ";"
                    << p.attribute.a
                    << ")";

            return ss.str();
        }

        std::string set_to_string(const std::vector<Path<A> > &set_seq_attr) const {
            std::ostringstream ss;

            ss << "[";

            for (std::size_t i = 0; i < set_seq_attr.size(); ++i) {
                if (i > 0) {
                    ss << " ";
                }

                ss << attr_to_string(set_seq_attr[i]);
            }

            ss << "]";

            return ss.str();
        }

        void write_csv(const std::string &filename, int rc, int dest) const {
            Logger l;
            l.log(INFO, "Writing Results");

            std::filesystem::path f = MakeOutputPath(filename, rc, dest);
            std::ofstream out(f);
            if (!out.is_open()) {
                throw std::runtime_error("Could not open trace file: " + filename);
            }

            out << "time,node,opt,set\n";

            for (const auto &e: entries) {
                out << e.time << ","
                        << e.node << ","
                        << "\"" << attr_to_string(e.opt_attr) << "\","
                        << "\"" << set_to_string(e.set_seq_attr) << "\""
                        << "\n";
            }
        }
    };
}

#endif

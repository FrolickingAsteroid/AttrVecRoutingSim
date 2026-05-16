#ifndef DATA_HPP
#define DATA_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "protocol/rpbob/state/path-segment.hpp"

namespace utils {

    enum class RegisterType {
        Set,
        NodeStab
    };

    template<class A>
    struct OptData {
        double time;
        int node;

        Protocol::State::Path<A> opt_attr;
    };

    template<class A>
    struct OptSetData : OptData<A> {
        std::vector<Protocol::State::Path<A>> set_seq_attr;
    };

    template<class A>
    struct Data {
        std::vector<OptData<A>> opt_entries;
        std::vector<OptSetData<A>> set_entries;

        void record(double time,
                    int node,
                    const Protocol::State::Path<A> &opt_attr) {
            opt_entries.push_back(OptData<A>{time, node, opt_attr});
        }

        void record(double time,
                    int node,
                    const Protocol::State::Path<A> &opt_attr,
                    const std::vector<Protocol::State::Path<A>> &set_seq_attr) {
            set_entries.push_back(OptSetData<A>{time, node, opt_attr, set_seq_attr});
        }
    };

    template<class A>
    class Registrar {
    public:
        using path = Protocol::State::Path<A>;

        Registrar(const std::filesystem::path &input_,
                  int beacons_,
                  int destination_,
                  RegisterType type_)

            : input(input_),
              beacons(beacons_),
              destination(destination_),
              type(type_) {
            if (type == RegisterType::NodeStab && destination < 0) {
                auto f = makeOutputPath("all", "stab");
                all_stab.open(f);

                if (!all_stab.is_open()) {
                    throw std::runtime_error("Could not open file: " + f.string());
                }

                all_stab << "dest,node,stab_time\n";
            }
        }

        void RegisterDestination(const int dest, const Data<A> &data) {
            if (type == RegisterType::Set) {
                RegisterSet(dest, data);
                return;
            }

            if (type == RegisterType::NodeStab) {
                RegisterNodeStab(dest, data);
            }
        }

    private:
        std::filesystem::path input;
        int beacons;
        int destination;
        RegisterType type;

        std::ofstream all_stab;

        std::filesystem::path makeOutputPath(const std::string &dest,
                                             const std::string &suffix) const {
            std::filesystem::path p = input;

            std::filesystem::path filename_stem = p.stem();
            std::filesystem::path dataset_id = p.parent_path().filename();
            std::filesystem::path dataset_family = p.parent_path().parent_path().filename();

            std::filesystem::path output_dir =
                    std::filesystem::path("data") / "output" / dataset_family / dataset_id;

            std::filesystem::create_directories(output_dir);

            std::string output_name =
                    filename_stem.string() + "_" + (dest == "all" ? "all" : "d" + dest) + "_rc" + std::to_string(beacons) + "_" + suffix + ".csv";

            return output_dir / output_name;
        }

        static std::string attrToString(const path &p) {
            std::ostringstream ss;

            ss << "("
               << p.attribute.n
               << ";"
               << p.attribute.a
               << ")";

            return ss.str();
        }

        static std::string setToString(const std::vector<path> &set_seq_attr) {
            std::ostringstream ss;

            ss << "[";

            for (std::size_t i = 0; i < set_seq_attr.size(); ++i) {
                if (i > 0) {
                    ss << " ";
                }

                ss << attrToString(set_seq_attr[i]);
            }

            ss << "]";

            return ss.str();
        }

        static std::ofstream openFile(const std::filesystem::path &f) {
            std::ofstream out(f);

            if (!out.is_open()) {
                throw std::runtime_error("Could not open file: " + f.string());
            }

            return out;
        }

        void RegisterSet(int dest, const Data<A> &data) const {
            auto f = makeOutputPath(std::to_string(dest), "set");
            auto out = openFile(f);

            out << "time,node,opt,set\n";

            for (const auto &e: data.set_entries) {
                out << e.time << ","
                    << e.node << ","
                    << "\"" << attrToString(e.opt_attr) << "\","
                    << "\"" << setToString(e.set_seq_attr) << "\""
                    << "\n";
            }
        }

        void WriteNodeStabRows(int dest,
                               const Data<A> &data,
                               std::ostream &out) const {
            std::unordered_map<int, double> last_change;

            for (const auto &e: data.opt_entries) {
                last_change[e.node] = e.time;
            }

            for (const auto &[node, time]: last_change) {
                out << dest << ","
                    << node << ","
                    << time << "\n";
            }
        }

        void RegisterNodeStab(int dest, const Data<A> &data) {
            if (destination < 0) {
                WriteNodeStabRows(dest, data, all_stab);
                return;
            }

            auto f = makeOutputPath(std::to_string(dest), "stab");
            auto out = openFile(f);

            out << "dest,node,stab_time\n";
            WriteNodeStabRows(dest, data, out);
        }
    };

}// namespace utils

#endif
#ifndef PATHS_HPP
#define PATHS_HPP

#include <vector>
#include <ostream>

namespace RoutingAlgebras {
    template<class Node>
    struct Path {
        std::vector<Node> path;

        Path() = default;

        Path(std::vector<Node> path_) : path(std::move(path_)) {
        }

        bool operator==(const Path &other) const {
            return path == other.path;
        }

        bool operator!=(const Path &other) const {
            return !(*this == other);
        }

        friend std::ostream &operator<<(std::ostream &os, const Path<Node> &p) {
            os << "[";
            for (size_t i = 0; i < p.path.size(); ++i) {
                os << p.path[i];
                if (i + 1 < p.path.size()) os << ", ";
            }
            return os << "]";
        }
    };
}
#endif

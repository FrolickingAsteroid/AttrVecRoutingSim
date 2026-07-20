#ifndef MP_PATH_SEGMENT_H
#define MP_PATH_SEGMENT_H

#include <vector>

namespace Protocol::Rpbob {
    template<class A>
    struct Path {
        A attr;
        std::vector<int> path;

        Path() = default;

        Path(int e, A a)
            : attr(a) {
            addAsEntry(e);
        }

        Path(std::vector<int> path_, A attr_)
            : attr(attr_)
              , path(std::move(path_)) {
        }

        void addAsEntry(const int &entry) {
            path.insert(path.begin(), entry);
        }

        A getAttr() const {
            return attr;
        }

        void addAttr(A &attr_) {
            attr = attr_;
        }

        bool operator==(const Path<A> &other) const {
            return attr == other.attr && path == other.path;
        }

        bool operator!=(const Path<A> &other) const {
            return !(*this == other);
        }

        friend std::ostream &operator<<(std::ostream &os, const Path<A> &p) {
            os << p.attr.a;
            return os;
        }
    };
} // namespace Protocol::Rpbob

#endif

#ifndef SEQ_WINDOW_HPP
#define SEQ_WINDOW_HPP

#include <optional>
#include <vector>

#include "path-segment.hpp"


namespace Protocol::State {
    template<class A, class Ord>
    class SequenceWindow {
    public:
        using path = Path<A>;

        SequenceWindow(const std::size_t N_)
            : N(N_),
              best_attr(path(-1, Ord::Null())) {
        }

        const path &GetBestAttr() const;

        const std::vector<path> &GetSetAttr() const;

        std::optional<path> CompareSetAttr(const path &n);

    private:
        std::size_t N;
        std::vector<path> set_seq_attr;
        path best_attr;
    };
} // namespace Protocol::State


#include "protocol/rpbob/state/seq-window.tpp"

#endif // SEQ_WINDOW_HPP

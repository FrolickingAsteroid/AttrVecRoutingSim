#ifndef SEQ_WINDOW_HPP
#define SEQ_WINDOW_HPP

#include <optional>
#include <vector>

#include "path-segment.hpp"
//#include "routing-algebras/extensions/seq-extension.hpp"
#include "routing-algebras/orders/sequenced-order.hpp"

namespace Protocol::Rpbob {
    template<class RA>
    class SequenceWindow {
        using A = RA::Attr;
        using Ord = RA::Ord;
        using path = Path<RA>;

    public:
        SequenceWindow(const std::size_t N_)
            : W(N_)
              , best_attr(path(-1, Ord::Null())) {
        }

        void SetOrigin(int n, int u);

        bool UpdateBestAttr();

        const path &GetBestAttr() const;

        const std::vector<path> &GetSetAttr() const;

        std::optional<path> CompareSetAttr(const path &c);

        std::vector<path> FailureNotif(std::pair<int, int> link_notif);

        static bool ContainsFailedLink(const path &p, int u, int v);

        std::optional<std::pair<int, int> > failed_link = std::nullopt;

    private:
        std::size_t W;
        int R;

        std::vector<path> set_seq_attr;
        std::vector<path> best_attr;

        template<class Pred>
        std::vector<path> RemoveMatching(Pred pred);
    };
} // namespace Protocol::Rpbob

#include "protocol/rpbob/state/seq-window.tpp"

#endif

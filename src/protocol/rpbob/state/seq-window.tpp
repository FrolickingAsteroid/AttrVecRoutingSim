#ifndef SEQ_WINDOW_TPP
#define SEQ_WINDOW_TPP

#include <algorithm>

#include "protocol/rpbob/state/seq-window.hpp"

namespace Protocol::State {
    template<class A, class Ord>
    const typename SequenceWindow<A, Ord>::path &
    SequenceWindow<A, Ord>::GetBestAttr() const {
        return best_attr;
    }

    template<class A, class Ord>
    const std::vector<typename SequenceWindow<A, Ord>::path> &SequenceWindow<A, Ord>::GetSetAttr() const {
        return set_seq_attr;
    }

    template<class A, class Ord>
    std::optional<typename SequenceWindow<A, Ord>::path>
    SequenceWindow<A, Ord>::CompareSetAttr(const path &n) {
        Ord ord(N);
        std::vector<path> kept;
        bool added = false;
        bool removed = false;

        // if the set is currently empty, add the newly arriving path
        if (set_seq_attr.empty()) {
            set_seq_attr.push_back(n);
            best_attr = n;
            return n;
        }

        // else compare each resident path to the new candidate
        for (auto it = set_seq_attr.rbegin(); it != set_seq_attr.rend(); ++it) {
            const auto &current = *it;

            //[TODO] replace == with tie logic related to string algebra
            // current attr < candidate attr
            if (current.attribute == n.attribute || ord(current.attribute, n.attribute)) {
                kept.push_back(current);
                removed = true;

                // current attr > candidate attr
            } else if (ord(n.attribute, current.attribute)) {
                if (!added) {
                    kept.push_back(n);
                    added = true;
                }

                // incomparable
            } else {
                kept.push_back(current);
            }
        }

        if (!added && removed == false) {
            kept.push_back(n);
        }

        // sort paths by sequence number
        std::sort(kept.begin(), kept.end(),
                  [](const path &l, const path &r) {
                      return l.attribute.n < r.attribute.n;
                  });

        if (set_seq_attr != kept) {
            set_seq_attr = std::move(kept);
            if (path new_best_attr = set_seq_attr.front(); new_best_attr.attribute != best_attr.attribute) {
                best_attr = new_best_attr;
            }
            return n;
        }

        /* update the best attr if it has changed
        if (path new_best_attr = set_seq_attr.front(); new_best_attr.attribute != best_attr.attribute) {
            best_attr = new_best_attr;
            return best_attr;
        }*/

        return std::nullopt;
    }
}// namespace Protocol::State
#endif// SEQ_WINDOW_TPP

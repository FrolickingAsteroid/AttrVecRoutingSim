#ifndef SEQ_WINDOW_TPP
#define SEQ_WINDOW_TPP

#include "protocol/rpbob/state/seq-window.hpp"
#include <algorithm>

namespace Protocol::Rpbob
{
template <class A, class Ord>
bool SequenceWindow<A, Ord>::FailureNotif(std::tuple<int, int, int> notif)
{
	auto [n, u, v] = notif;
	bool erased    = false;

	if (set_seq_attr.empty()) {
		return erased;
	}

	if (best_attr.attr.n > n) {
		return erased;
	}

	auto contains_failed_link = [u, v](const path &p) {
		const auto &nodes = p.path;

		if (nodes.size() < 2) {
			return false;
		}

		for (std::size_t i = 0; i + 1 < nodes.size(); ++i) {
			const int a = nodes[i];
			const int b = nodes[i + 1];

			if ((a == u && b == v) || (a == v && b == u)) {
				return true;
			}
		}

		return false;
	};

	set_seq_attr.erase(std::remove_if(set_seq_attr.begin(),
					  set_seq_attr.end(),
					  [&](const path &p) {
						  if (!contains_failed_link(p)) {
							  return false;
						  }
						  if (p == best_attr) {
							  return false;
						  }
						  erased = true;
						  return true;
					  }),
			   set_seq_attr.end());

	return erased;
}
template <class A, class Ord>
const SequenceWindow<A, Ord>::path &SequenceWindow<A, Ord>::GetBestAttr() const
{
	return best_attr;
}

template <class A, class Ord>
const std::vector<typename SequenceWindow<A, Ord>::path> &
SequenceWindow<A, Ord>::GetSetAttr() const
{
	return set_seq_attr;
}

template <class A, class Ord> void SequenceWindow<A, Ord>::SetOrigin(int n, int u)
{
	best_attr = path(u, Ord::Neutral(n));
}

template <class A, class Ord>
std::optional<typename SequenceWindow<A, Ord>::path>
SequenceWindow<A, Ord>::CompareSetAttr(const path &c)
{
	Ord ord(N);

	// if anything in the set is better than the current candidate, discard it
	for (const auto &current : set_seq_attr)
		if (ord(current.attr, c.attr) || c.attr == current.attr)
			return std::nullopt;

	// remove anything worse than the candidate
	set_seq_attr.erase(std::remove_if(set_seq_attr.begin(),
					  set_seq_attr.end(),
					  [&](const path &current) {
						  return ord(c.attr, current.attr);
					  }),
			   set_seq_attr.end());

	set_seq_attr.push_back(c);

	// sort by sequence number
	std::sort(set_seq_attr.begin(),
		  set_seq_attr.end(),
		  [](const path &l, const path &r) { return l.attr.n < r.attr.n; });

	// update best
	path new_best = set_seq_attr.front();
	if (new_best.attr != best_attr.attr)
		best_attr = new_best;

	return c;
}
}
#endif // SEQ_WINDOW_TPP

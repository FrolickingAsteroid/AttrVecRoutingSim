#ifndef SEQ_WINDOW_TPP
#define SEQ_WINDOW_TPP

#include "protocol/rpbob/state/seq-window.hpp"
#include <algorithm>

namespace Protocol::Rpbob
{
template <class A, class Ord>
const SequenceWindow<A, Ord>::path &SequenceWindow<A, Ord>::GetBestAttr() const
{
	return best_attr;
}

template <class A, class Ord>
const std::vector<typename SequenceWindow<A, Ord>::path> &SequenceWindow<A, Ord>::GetSetAttr() const
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
	set_seq_attr.erase(
		std::remove_if(set_seq_attr.begin(),
			       set_seq_attr.end(),
			       [&](const path &current) { return ord(c.attr, current.attr); }),
		set_seq_attr.end());

	set_seq_attr.push_back(c);

	// sort by sequence number
	std::sort(set_seq_attr.begin(), set_seq_attr.end(), [](const path &l, const path &r) {
		return l.attr.n < r.attr.n;
	});

	// update best
	path new_best = set_seq_attr.front();
	if (new_best.attr != best_attr.attr)
		best_attr = new_best;

	return c;
}
}
#endif // SEQ_WINDOW_TPP

#ifndef SEQ_WINDOW_TPP
#define SEQ_WINDOW_TPP

#include <algorithm>

#include "protocol/rpbob/state/seq-window.hpp"

namespace Protocol::Rpbob
{
template <class RA> bool SequenceWindow<RA>::ContainsFailedLink(const path &p, int u, int v)
{
	const auto &nodes = p.path;
	if (nodes.size() < 2)
	{
		return false;
	}
	for (std::size_t i = 0; i + 1 < nodes.size(); ++i)
	{
		const int a = nodes[i];
		const int b = nodes[i + 1];
		if ((a == u && b == v) || (a == v && b == u))
		{
			return true;
		}
	}
	return false;
}

template <class RA>
template <class Pred>
std::vector<typename SequenceWindow<RA>::path> SequenceWindow<RA>::RemoveMatching(Pred pred)
{
	std::vector<path> removed;

	if (set_seq_attr.empty())
	{
		return removed;
	}
	set_seq_attr.erase(std::remove_if(set_seq_attr.begin(), set_seq_attr.end(),
					  [&](const path &p) {
		if (!pred(p))
		{
			return false;
		}
		removed.push_back(p);
		return true;
	}),
			   set_seq_attr.end());

	return removed;
}

// Node that detects the link failure: remove any path using edge (u, v)
template <class RA>
std::vector<typename SequenceWindow<RA>::path> SequenceWindow<RA>::FailureNotif(std::pair<int, int> link_notif)
{
	auto [u, v] = link_notif;

	return RemoveMatching([u, v](const path &p) {
		return ContainsFailedLink(p, u, v);
	});
}

// Reception of a failure notif: remove paths that match
template <class RA>
std::vector<typename SequenceWindow<RA>::path> SequenceWindow<RA>::FailureNotif(const path &path_notif)
{
	return RemoveMatching([&path_notif](const path &p) {
		return p.path == path_notif.path;
	});
}

template <class RA> const SequenceWindow<RA>::path &SequenceWindow<RA>::GetBestAttr() const
{
	return best_attr;
}

template <class RA> const std::vector<typename SequenceWindow<RA>::path> &SequenceWindow<RA>::GetSetAttr() const
{
	return set_seq_attr;
}

template <class RA> void SequenceWindow<RA>::SetOrigin(int n, int u)
{
	best_attr = path(u, Ord::Neutral(n));
	set_seq_attr.assign(1, best_attr);
}

template <class RA> std::optional<typename SequenceWindow<RA>::path> SequenceWindow<RA>::CompareSetAttr(const path &c)
{
	if (c.isInvalid())
	{
		return std::nullopt;
	}

	Ord ord(N);

	// if anything in the set is better than the current candidate, discard it
	for (const auto &current : set_seq_attr)
		if (ord(current.attr, c.attr) || c.attr == current.attr)
			return std::nullopt;

	// remove anything worse than the candidate
	set_seq_attr.erase(std::remove_if(set_seq_attr.begin(), set_seq_attr.end(),
					  [&](const path &current) {
		return ord(c.attr, current.attr);
	}),
			   set_seq_attr.end());

	set_seq_attr.push_back(c);

	UpdateBestAttr();

	return c;
}

template <class RA> void SequenceWindow<RA>::UpdateBestAttr()
{
	// sort by sequence number
	if (set_seq_attr.empty())
	{
		best_attr = path(-1, Ord::Null());
		return;
	}
	std::sort(set_seq_attr.begin(), set_seq_attr.end(), [](const path &l, const path &r) {
		return l.attr.n < r.attr.n;
	});

	// update best
	path new_best = set_seq_attr.front();
	if (new_best.attr != best_attr.attr)
		best_attr = new_best;
}

template <class RA> std::ostream &operator<<(std::ostream &os, const SequenceWindow<RA> &sw)
{
	const auto &best = sw.GetBestAttr();

	os << "best attr : " << best << " (";
	for (std::size_t i = 0; i < best.path.size(); ++i)
	{
		os << best.path[i];
		if (i + 1 < best.path.size())
		{
			os << ",";
		}
	}
	os << ") ";

	os << "[";
	const auto &set = sw.GetSetAttr();
	for (std::size_t i = 0; i < set.size(); ++i)
	{
		os << set[i].attr;
		if (i + 1 < set.size())
		{
			os << ",";
		}
	}
	os << "]";
	return os;
}
} // namespace Protocol::Rpbob
#endif // SEQ_WINDOW_TPP

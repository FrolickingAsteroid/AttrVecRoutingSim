#ifndef SEQ_WINDOW_TPP
#define SEQ_WINDOW_TPP

#include <algorithm>

#include "protocol/rpbob/state/seq-window.hpp"
#include "routing-algebras/orders/comparison-vocab.hpp"

namespace Protocol::Rpbob
{

// ---------------------------------------------------------------------------- *
//
// Getters and Setters
//
// ---------------------------------------------------------------------------- *

template <class RA>
std::vector<typename SequenceWindow<RA>::path> SequenceWindow<RA>::GetBestAttr() const
{
	return opt;
}

template <class RA>
const std::vector<typename SequenceWindow<RA>::path> &SequenceWindow<RA>::GetSetAttr() const
{
	return wind;
}

template <class RA>
void SequenceWindow<RA>::SetOrigin(int n, int u)
{
	using base = RA::Base;

	auto base_attr = base::Neutral(u);

	A attr(n, base_attr);

	opt  = {path(attr)};
	wind = opt;
}

// ---------------------------------------------------------------------------- *
//
// Fail Notif
//
// ---------------------------------------------------------------------------- *

template <class RA>
bool SequenceWindow<RA>::ContainsFailedLink(const path &p, int u, int v)
{
	// TODO: Make this less convoluted pt2
	const auto &nodes = p.attr.b.getPath();
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

	if (wind.empty())
	{
		return removed;
	}
	wind.erase(std::remove_if(wind.begin(), wind.end(),
				  [&](const path &p) {
		if (!pred(p))
		{
			return false;
		}
		removed.push_back(p);
		return true;
	}),
		   wind.end());

	return removed;
}

template <class RA>
std::vector<typename SequenceWindow<RA>::path> SequenceWindow<RA>::FailureNotif(std::pair<int, int> link_notif)
{
	auto [u, v] = link_notif;

	return RemoveMatching([u, v](const path &p) {
		return ContainsFailedLink(p, u, v);
	});
}

// ---------------------------------------------------------------------------- *
//
// Attribute comparison
//
// ---------------------------------------------------------------------------- *

template <class RA>
std::optional<typename SequenceWindow<RA>::path> SequenceWindow<RA>::CompareSetAttr(const path &c)
{
	// reject empty paths
	if (c.isInvalid())
	{
		return std::nullopt;
	}

	const int n = c.attr.a;

	// reject anything already outside the window
	if (n < R - static_cast<int>(W))
	{
		return std::nullopt;
	}

	Ord ord;

	// if anything in the set is better (or equals) the candidate, discard the candidate
	for (const auto &current : wind)
	{
		if (auto result = ord(current.attr, c.attr); RoutingAlgebras::beq(result))
		{
			return std::nullopt;
		}
	}

	// remove anything that is worse than the candidate
	wind.erase(std::remove_if(wind.begin(), wind.end(),
				  [&](const path &current) {
		return RoutingAlgebras::beq(ord(c.attr, current.attr));
	}),
		   wind.end());

	wind.push_back(c);

	// advance the window if n > R
	if (n > R)
	{
		R = n;
		wind.erase(std::remove_if(wind.begin(), wind.end(),
					  [&](const path &p) {
			return p.attr.a < R - static_cast<int>(W);
		}),
			   wind.end());
	}

	UpdateBestAttr();

	return c;
}

template <class RA>
bool SequenceWindow<RA>::UpdateBestAttr()
{
	std::vector<path> new_opt{};

	std::sort(wind.begin(), wind.end(), [](const path &l, const path &r) {
		return l.attr.a < r.attr.a;
	});

	if (!wind.empty())
	{
		BaseOrd base_ord;

		for (const auto &a : wind)
		{
			bool dom = false;
			for (const auto &b : wind)
			{
				if (base_ord(a.attr.b, b.attr.b) == RoutingAlgebras::Comp::Worse)
				{
					dom = true;
					break;
				}
			}
			if (!dom)
			{
				new_opt.push_back(a);
			}
		}
	}

	auto changed = CompareOpt(new_opt, opt);
	opt          = new_opt;

	return changed;
}
} // namespace Protocol::Rpbob
#endif // SEQ_WINDOW_TPP

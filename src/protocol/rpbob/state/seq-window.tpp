#ifndef SEQ_WINDOW_TPP
#define SEQ_WINDOW_TPP

#include <algorithm>

#include "protocol/rpbob/state/seq-window.hpp"
#include "routing-algebras/orders/comparison-vocab.hpp"
#include "utils/helpers/helpers.hpp"

namespace Protocol::Rpbob
{

// ---------------------------------------------------------------------------- *
//
// Getters and Setters
//
// ---------------------------------------------------------------------------- *

template <class RA>
std::vector<typename SequenceWindow<RA>::path> SequenceWindow<RA>::GetOpt() const
{
	return opt;
}

template <class RA>
void SequenceWindow<RA>::ClearOpt()
{
	opt.clear();
}

template <class RA>
const std::vector<typename SequenceWindow<RA>::path> &SequenceWindow<RA>::GetWind() const
{
	return wind;
}

template <class RA>
void SequenceWindow<RA>::SetOrigin(int n, int u)
{

	A e = RA::Neutral(n, u);

	opt  = {path(e)};
	wind = opt;
}

// ---------------------------------------------------------------------------- *
//
// Fail Notif
//
// ---------------------------------------------------------------------------- *

template <class RA>
bool SequenceWindow<RA>::AddFailedLink(int u, int v)
{
	if (!failed_link)
	{
		failed_link.emplace();
	}
	if (std::ranges::find(*failed_link, std::pair{u, v}) != failed_link->end())
	{
		return false;
	}

	failed_link->emplace_back(u, v);

	return true;
}
template <class RA>
bool SequenceWindow<RA>::ContainsFailedLink(const path &p, int u, int v)
{
	A seq_attr = p.getAttr();

	const auto &nodes = seq_attr.getAttr().getPath();
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
bool SequenceWindow<RA>::ContainsFailedLink(const path &p) const
{
	return failed_link && std::ranges::any_of(*failed_link, [&](const auto &l) {
		auto [u, v] = l;
		return ContainsFailedLink(p, u, v);
	});
}

template <class RA>
std::vector<typename SequenceWindow<RA>::path> SequenceWindow<RA>::FailureNotif(std::pair<int, int> link_notif)
{
	auto [u, v] = link_notif;

	std::vector<path> removed;

	if (wind.empty())
	{
		return removed;
	}
	wind.erase(std::remove_if(wind.begin(), wind.end(),
				  [&](const path &p) {
		if (!ContainsFailedLink(p, u, v))
		{
			return false;
		}
		removed.push_back(p);
		return true;
	}),
		   wind.end());

	return removed;
}

// ---------------------------------------------------------------------------- *
//
// Attribute comparison
//
// ---------------------------------------------------------------------------- *

template <class RA>
std::optional<typename SequenceWindow<RA>::path> SequenceWindow<RA>::CompareWind(const path &c)
{
	A candidate = c.getAttr();

	const int n = candidate.getSeqNum();

	// reject anything already outside the window
	if (n <= R - W)
	{
		return std::nullopt;
	}

	Ord ord;

	// if anything in the set is better (or equals) the candidate, discard the candidate
	for (const auto &current : wind)
	{
		if (auto result = ord(current.getAttr(), candidate); RoutingAlgebras::BetterOrEqual(result))
		{
			return std::nullopt;
		}
	}

	// remove anything that is worse than the candidate
	wind.erase(std::remove_if(wind.begin(), wind.end(),
				  [&](const path &current) {
		return RoutingAlgebras::BetterOrEqual(ord(candidate, current.getAttr()));
	}),
		   wind.end());

	wind.push_back(c);

	// advance the window if n > R
	if (n > R)
	{
		R = n;
		wind.erase(std::remove_if(wind.begin(), wind.end(),
					  [&](const path &p) {
			return p.attr.getSeqNum() <= R - W;
		}),
			   wind.end());
	}

	UpdateOpt();

	return c;
}

template <class RA>
bool SequenceWindow<RA>::UpdateOpt()
{
	std::vector<path> new_opt{};

	std::sort(wind.begin(), wind.end(), [](const path &l, const path &r) {
		return l.getAttr().getSeqNum() < r.getAttr().getSeqNum();
	});

	if (!wind.empty())
	{
		typename RA::Base::Ord ord;

		for (const auto &a : wind)
		{
			bool dom = false;
			for (const auto &b : wind)
			{
				if (ord(a.attr.getAttr(), b.attr.getAttr()) == RoutingAlgebras::Comp::Worse)
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
	else
	{
		new_opt = {path(RA::Null(-1))};
	}

	auto changed = utils::helpers::CompareOpt<path>(new_opt, opt);
	opt          = new_opt;

	return changed;
}

} // namespace Protocol::Rpbob
#endif // SEQ_WINDOW_TPP

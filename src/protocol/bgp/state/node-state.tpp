#ifndef BGP_NODE_STATE_TPP
#define BGP_NODE_STATE_TPP

#include "protocol/bgp/state/node-state.hpp"

namespace Protocol::BGP
{

template <class Attr>
bool BGPNodeState<Attr>::ProcessBest(int v)
{
	// Ord ord;
	if (aTab.at(v) < E)
	{
		// aTab[v] is strictly better than current E
		succ = v;
		Av   = aTab.at(v);
	}
	else if (succ == v && E < aTab.at(v))
	{
		// current successor got worse
		fullScan();
	}
	else if (succ == v)
	{
		// Successor's attribute changed but stayed preferred: update in place.
		Av = aTab.at(v);
	}
	// else: unrelated neighbor, not better than E

	return commitAv();
}

template <class Attr>
bool BGPNodeState<Attr>::ProcessBest()
{
	fullScan();
	return commitAv();
}

template <class Attr>
void BGPNodeState<Attr>::fullScan()
{
	Av   = attr::Invalid();
	succ = -1;
	for (const auto &[neighbor, learned] : aTab)
	{
		if (learned < Av || (learned == Av && neighbor < succ))
		{
			Av   = learned;
			succ = neighbor;
		}
	}
}

template <class Attr>
bool BGPNodeState<Attr>::commitAv()
{
	if (Av != E)
	{
		E = Av;
		return true;
	}
	return false;
}

} // namespace Protocol::BGP

#endif

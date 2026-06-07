#ifndef BGP_NODE_STATE_TPP
#define BGP_NODE_STATE_TPP

#include "protocol/bgp/state/node-state.hpp"

namespace Protocol::BGP
{

template <class A, class Ext, class Ord> bool BGPNodeState<A, Ext, Ord>::ProcessBest(int v)
{
	Ord ord;
	if (aTab.at(v) < E) {
		// aTab[v] is strictly better than current E
		succ = v;
		Av   = aTab.at(v);
	} else if (succ == v && E < aTab.at(v)) {
		// current successor got worse
		fullScan();
	} else if (succ == v) {
		// Successor's attribute changed but stayed preferred: update in place.
		Av = aTab.at(v);
	}
	// else: unrelated neighbor, not better than E

	return commitAv();
}

template <class A, class Ext, class Ord> bool BGPNodeState<A, Ext, Ord>::ProcessBest()
{
	fullScan();
	return commitAv();
}

template <class A, class Ext, class Ord> void BGPNodeState<A, Ext, Ord>::fullScan()
{
	Av   = O;
	succ = -1;
	for (const auto &[neighbor, learned] : aTab) {
		if (learned < Av) {
			Av   = learned;
			succ = neighbor;
		}
	}
}

template <class A, class Ext, class Ord> bool BGPNodeState<A, Ext, Ord>::commitAv()
{
	if (Av != E) {
		E = Av;
		return true;
	}
	return false;
}

}

#endif

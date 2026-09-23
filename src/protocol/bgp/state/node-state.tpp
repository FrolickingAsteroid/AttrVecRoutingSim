#ifndef BGP_NODE_STATE_TPP
#define BGP_NODE_STATE_TPP

#include "protocol/bgp/state/node-state.hpp"
#include "routing-algebras/orders/comparison-vocab.hpp"

namespace Protocol::BGP
{

template <class RA, class AttrType>
bool BGPNodeState<RA, AttrType>::ProcessBest(int v)
{
	Ord ord;
	if (ord(aTab.at(v).getAttr(), E.getAttr()) == RoutingAlgebras::Comp::Better)
	{
		// aTab[v] is strictly better than current E
		succ = v;
		Av   = aTab.at(v);
	}
	else if (succ == v && ord(aTab.at(v).getAttr(), E.getAttr()) == RoutingAlgebras::Comp::Worse)
	{
		// current successor got worse
		fullScan();
	}
	else if (succ == v)
	{
		// Successor's RAibute changed but stayed preferred: update in place.
		Av = aTab.at(v);
	}
	return commitAv();
}

template <class RA, class AttrType>
bool BGPNodeState<RA, AttrType>::ProcessBest()
{
	fullScan();
	return commitAv();
}

template <class RA, class AttrType>
void BGPNodeState<RA, AttrType>::fullScan()
{
	Ord ord;
	Av   = path::Invalid();
	succ = -1;
	for (const auto &[neighbor, learned] : aTab)
	{
		if (ord(learned.getAttr(), Av.getAttr()) == RoutingAlgebras::Comp::Better)
		{
			Av   = learned;
			succ = neighbor;
		}
	}
}

template <class RA, class AttrType>
bool BGPNodeState<RA, AttrType>::commitAv()
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

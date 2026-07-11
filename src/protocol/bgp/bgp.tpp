#ifndef BGP_TPP
#define BGP_TPP

#include "protocol/bgp/bgp.hpp"

namespace Protocol::BGP
{

template <class RA> void BGP<RA>::process()
{
	for (int u = 0; u < graph.V; ++u) {
		auto it	  = data.finalValue.find(u);
		auto best = s[u].GetElected();

		if (best.attribute == Ord::Null() && data.log == false) {
			continue;
		}

		if (it == data.finalValue.end() || best != it->second) {
			data.recordChange(u, cal.now(), best);
		}
	}
}

template <class RA> void BGP<RA>::LinkFailure(int u, int v)
{
	graph.removeEdge(u, v);
	graph.removeEdge(v, u);
	data.log = true;
	data.setOffset(cal.now());

	for (auto [node, neighbor] : { std::pair{ u, v }, std::pair{ v, u } }) {
		if (s[node].succ == neighbor) {
			s[node].RemoveNeighbor(neighbor);
			if (s[node].ProcessBest()) {
				this->Send(node, s[node].GetElected());
				data.recordChange(node, cal.now(), s[node].GetElected());
			}
		}
	}
}

template <class RA> void BGP<RA>::NodeDown(int t)
{
	for (auto &[r, edge] : graph.adjListIn[t]) {
		if (s[r].succ == t) {
			s[r].RemoveNeighbor(t);
			graph.deactivate(r, t);
			if (s[r].ProcessBest()) {
				this->Send(r, s[r].GetElected());
				data.recordChange(r, cal.now(), s[r].GetElected());
			}
		}
	}
	data.log = true;
	data.setOffset(cal.now());
}

template <class RA> void BGP<RA>::DestinationAnnouncement(int t)
{
	// resetCycleState();

	s[t].SetOrigin(t);
	this->Send(t, s[t].O);
}

template <class RA> void BGP<RA>::MsgReception(int u, int v, attr received)
{
	if (graph.adjListOut[u].find(v) == graph.adjListOut[u].end())
		return;

	attr lw(u, std::get<2>(graph.adjListOut[u].at(v)));
	attr extended = lw + received;
	s[u].UpdateNeighbor(v, extended);
	if (s[u].ProcessBest(v)) {
		this->Send(u, s[u].GetElected());
		//converged();
	}
}

// ─── cycle / correctness observation ─────────────────────────────────────────
/*
template <class A, class Ord, class Ext> std::vector<int> &BGP<A, Ord, Ext>::CycleBlackHoleTimes()
{
	for (int u = 0; u < static_cast<int>(s.size()); ++u)
		if (!s[u].HasRoute())
			timeInCycle[u] = -1;
	return timeInCycle;
}

template <class A, class Ord, class Ext> std::vector<int> &BGP<A, Ord, Ext>::CycleBlackHoleChanges()
{
	for (int u = 0; u < static_cast<int>(s.size()); ++u)
		if (!s[u].HasRoute())
			nCycles[u] = -1;
	return nCycles;
}


template <class A, class Ord, class Ext> void BGP<A, Ord, Ext>::resetCycleState()
{
	std::fill(nCycles.begin(), nCycles.end(), 0);
	std::fill(inCycle.begin(), inCycle.end(), false);
	std::fill(cycleFormed.begin(), cycleFormed.end(), 0);
	std::fill(timeInCycle.begin(), timeInCycle.end(), 0);
	std::fill(firstChange.begin(), firstChange.end(), false);
}

template <class A, class Ord, class Ext> void BGP<A, Ord, Ext>::checkCycles(int t)
{
	// Mirrors the check block at the bottom of processEvent.
	// Call this after any event if cycle tracking is enabled.
	int n = static_cast<int>(s.size());
	std::vector<bool> vis(n, false);
	std::vector<bool> inStack(n, false);

	for (int u = 0; u < n; ++u) {
		std::fill(vis.begin(), vis.end(), false);
		if (auxCheckCycle(u, vis, inStack)) {
			if (changeAttr[u]) {
				++nCycles[u];
				changeAttr[u] = false;
			}
			if (!inCycle[u]) {
				inCycle[u] = true;
				cycleFormed[u] = t;
			}
		} else {
			if (inCycle[u]) {
				timeInCycle[u] += (t - cycleFormed[u]);
				inCycle[u] = false;
			}
		}
	}
}

template <class A, class Ord, class Ext>
bool BGP<A, Ord, Ext>::auxCheckCycle(int u, std::vector<bool> &vis, std::vector<bool> &inStack)
{
	// Mirrors auxCheckCycle from bgpOneDest.cpp.
	// A node participates in a cycle if it has not yet had its first change,
	// or if following successors leads back to a visited node.
	if (!firstChange[u])
		return false;

	if (!s[u].HasRoute()) {
		inStack[u] = true;
		vis[u] = true;
		return true;
	}

	if (inStack[u] || vis[u]) {
		inStack[u] = true;
		vis[u] = true;
		return true;
	}

	vis[u] = true;

	// Follow all out-neighbors whose learned attribute is not worse than
	// u's elected — i.e. neighbors u might still consider switching to.
	for (auto &[v, edge] : graph.adjListOut[u]) {
		auto it = s[u].GetAttrTable().find(v);
		if (it == s[u].GetAttrTable().end())
			continue;
		if (!(s[u].GetElected() < it->second)) {
			if (inStack[v] || vis[v] || auxCheckCycle(v, vis, inStack)) {
				inStack[u] = true;
				return true;
			}
		}
	}

	inStack[u] = false;
	vis[u] = false;
	return false;
}
*/

}
#endif
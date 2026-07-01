#ifndef RCN_TPP
#define RCN_TPP

#include "protocol/bgp-rcn/bgp-rcn.hpp"

namespace Protocol::RCN
{
template <class RA> void RCN<RA>::process()
{
	for (int u = 0; u < graph.V; ++u) {
		auto it	  = data.finalValue.find(u);
		auto best = s[u].GetElected();

		if (best.attribute == Ord::Null()) {
			continue;
		}

		if (it == data.finalValue.end() ||
		    best.attribute != it->second.attribute) {
			data.recordChange(u, cal.now(), best);
		}
	}
}

template <class RA> void RCN<RA>::LinkFailure(int u, int v)
{
	graph.removeEdge(u, v);
	graph.removeEdge(v, u);

	data.log = true;
	data.setOffset(cal.now());

	for (auto [node, neighbor] : { std::pair{ u, v }, std::pair{ v, u } }) {
		if (s[node].succ == neighbor) {
			s[node].RemoveNeighbor(neighbor);
			if (s[node].ProcessBest()) {
				rcn notif{ node, ++s[u].t };

				this->Send(node, Msg{ s[node].GetElected(), notif });
				data.recordChange(node, cal.now(), s[node].GetElected());
			}
		}
	}
}

template <class RA> void RCN<RA>::DestinationAnnouncement(int t)
{
	s[t].SetOrigin(t);
	this->Send(t, Msg{ s[t].O });
}

template <class RA> void RCN<RA>::MsgReception(int u, int v, Msg received)
{
	if (graph.adjListOut[u].find(v) == graph.adjListOut[u].end())
		return;

	if (s[u].UpdateSeqTable(received)) {
		attr lw(u, std::get<2>(graph.adjListOut[u].at(v)), s[u].t);
		attr extended = lw + received.route;
		s[u].UpdateNeighbor(v, extended);
		if (s[u].ProcessBest(v)) {
			++s[u].t;
			if (received.RCN.has_value()) {
				this->Send(u, Msg{ s[u].GetElected(), *received.RCN });
			} else {
				this->Send(u, Msg{ s[u].GetElected() });
			}
		}
	}
}
}

#endif
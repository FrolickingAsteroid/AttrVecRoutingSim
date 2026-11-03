#ifndef RCN_TPP
#define RCN_TPP

#include "protocol/bgp-rcn/bgp-rcn.hpp"

namespace Protocol::RCN
{
template <class RA>
void RCN<RA>::process()
{
	for (int u = 0; u < graph.V; ++u)
	{
		auto it   = data.finalValue.find(u);
		auto best = s[u].GetElected(u);

		if (best.attribute == RA::Null() && it == data.finalValue.end())
		{
			continue;
		}

		if (it == data.finalValue.end() || best != it->second.front())
		{
			data.recordChange(u, cal.now(), best);
		}
	}
}

// ---------------------------------------------------------------------------- *
//
// Link Failure Handling
//
// ---------------------------------------------------------------------------- *

template <class RA>
void RCN<RA>::LinkFailure(int u, int v)
{
	graph.deactivate(u, v);

	data.log = true;
	data.setOffset(cal.now());

	for (auto [node, neighbor] : {
		 std::pair{u, v},
                 std::pair{v, u}
        })
	{
		s[node].RemoveNeighbor(neighbor);
		if (s[node].succ == neighbor)
		{
			if (s[node].ProcessBest())
			{
				rcn notif{node, ++s[node].t};

				this->Send(node, Msg{s[node].GetElected(node), notif});
				data.recordChange(node, cal.now(), s[node].GetElected(node));
			}
		}
	}
}

// ---------------------------------------------------------------------------- *
//
// Message Processing
//
// ---------------------------------------------------------------------------- *

template <class RA>
void RCN<RA>::DestinationAnnouncement(int t)
{
	s[t].SetOrigin(t);
	this->Send(t, Msg{s[t].O});
}

template <class RA>
void RCN<RA>::MsgReception(int u, int v, Msg received)
{
	if (!graph.edgeState(v, u))
	{
		return;
	}

	RCNAttr lw(u, graph.edgeWeight(u, v).buildLinkAttr(u), s[u].t);
	RCNAttr extended = lw + received.route;
	s[u].Update(v, extended);

	s[u].UpdateSeqTable(received, u);

	if (s[u].ProcessBest(v))
	{
		auto l    = s[u];
		auto send = s[u].GetElected(u);

		if (received.RCN.has_value())
		{

			this->Send(u, Msg{send, *received.RCN});
		}
		else
		{
			this->Send(u, Msg{send});
		}
	}
}
} // namespace Protocol::RCN

#endif

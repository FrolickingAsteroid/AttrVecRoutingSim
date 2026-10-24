#ifndef BGP_TPP
#define BGP_TPP

#include "protocol/bgp/bgp.hpp"
#include "utils/ui/logger.hpp"

namespace Protocol::BGP
{
template <class RA>
void BGP<RA>::process()
{
	for (int u = 0; u < graph.V; ++u)
	{
		auto it   = data.finalValue.find(u);
		auto best = s[u].GetElected();

		// skip nodes that have yet to register an estimate toward t
		if (best.attribute == RA::Null() && it == data.finalValue.end())
		{
			continue;
		}

		if (cal.now() >= s[u].MRAI && (it == data.finalValue.end() || best != it->second.front()))
		{
			// A msg is sent each time the estimate to the destination changes
			s[u].MRAI = cal.now() + 1;
			this->Send(u, s[u].GetElected());
			data.recordChange(u, cal.now(), best);
		}
	}
}

template <class RA>
void BGP<RA>::LinkFailure(int u, int v)
{
	graph.deactivate(u, v);
	data.log = true;
	data.setOffset(cal.now());

	LOG_DEBUG << "Time of Failure: " << cal.now() << "\n";

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
				this->Send(node, s[node].GetElected());
				data.recordChange(node, cal.now(), s[node].GetElected());
			}
		}
	}
}

template <class RA>
void BGP<RA>::NodeWithdraw(int u)
{
	graph.deactivate(u);
	data.log = true;
	data.setOffset(cal.now());

	LOG_DEBUG << "Time of Failure: " << cal.now() << "\n";

	for (auto &[node, edge] : graph.adjListIn[u])
	{
		if (s[node].succ == u)
		{
			s[node].RemoveNeighbor(u);
			if (s[node].ProcessBest())
			{
				this->Send(node, s[node].GetElected());
				data.recordChange(node, cal.now(), s[node].GetElected());
			}
		}
	}
}

template <class RA>
void BGP<RA>::NodeDown(int t)
{
	for (auto &[r, edge] : graph.adjListIn[t])
	{
		if (s[r].succ == t)
		{
			s[r].RemoveNeighbor(t);
			graph.deactivate(r, t);
			if (s[r].ProcessBest())
			{
				this->Send(r, s[r].GetElected());
				data.recordChange(r, cal.now(), s[r].GetElected());
			}
		}
	}
	data.log = true;
	data.setOffset(cal.now());
}

template <class RA>
void BGP<RA>::DestinationAnnouncement(int t)
{
	s[t].SetOrigin(t);
	this->Send(t, s[t].O);
}

template <class RA>
void BGP<RA>::MsgReception(int u, int v, BGPAttr received)
{
	if (!graph.edgeState(v, u))
	{
		return;
	}

	BGPAttr lw(graph.edgeWeight(u, v).buildLinkAttr(u));
	BGPAttr extended = lw + received;
	
	s[u].UpdateNeighbor(v, extended);
	s[u].ProcessBest(v);
}
} // namespace Protocol::BGP
#endif

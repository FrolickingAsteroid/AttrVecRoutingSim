#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "graph/graph.hpp"
#include "scheduler/calendar.hpp"
#include "utils/data.hpp"

namespace Protocol
{

template <class A, class R> struct IProtocol {
	Graphs::Graph<A> &graph;
	scheduler::Calendar cal;
	utils::Data<R> data;

	explicit IProtocol(Graphs::Graph<A> &graph_)
		: graph(graph_)
	{
	}
	virtual ~IProtocol() = default;

	void Run()
	{
		cal.run();
	}

	void LinkFailure(int u, int v)
	{
		graph.deactivate(u, v);
	}

	uint64_t Send(int t, R msg)
	{
		uint64_t count = 0;
		for (auto &[r, edge] : graph.adjListIn[t]) {
			auto &[delay, active, attr] = edge;

			if (!active)
				continue;

			cal.schedule(cal.now() + delay,
				     [this, r, t, msg]() mutable {
					     this->MsgReception(r, t, msg);
				     });
			count++;
		}
		return count;
	}

	virtual void MsgReception(int u, int v, R msg) = 0;

	virtual void DestinationAnnouncement(int t, int n, double period,
					     int max) = 0;
};

} // namespace Protocol
#endif
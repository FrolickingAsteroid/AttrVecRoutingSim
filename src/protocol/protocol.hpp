#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "graph/graph.hpp"
#include "utils/stats/data.hpp"
#include "scheduler/calendar.hpp"

namespace Protocol
{

template <class A, class R> struct IProtocol {
	Graphs::Graph<A>     &graph;
	scheduler::Calendar   cal;
	utils::stats::Data<R> data;

	explicit IProtocol(Graphs::Graph<A> &graph_)
		: graph(graph_)
		, data(graph_.V)
	{
	}
	virtual ~IProtocol() = default;

	void Run()
	{
		cal.run([this]() { process(); });
	}

	virtual void MsgReception(int u, int v, R msg) = 0;
	virtual void LinkFailure(int u, int v)	       = 0;

	// callbacks
	virtual void process()
	{
	}

	void Send(int t, R msg)
	{
		for (auto &[r, edge] : graph.adjListIn[t]) {
			auto &[delay, active, attr] = edge;

			if (!active)
				continue;

			cal.schedule(cal.now() + delay, [this, r, t, msg]() mutable { this->MsgReception(r, t, msg); });
			data.recordMessage(t);
		}
	}
};

} // namespace Protocol
#endif
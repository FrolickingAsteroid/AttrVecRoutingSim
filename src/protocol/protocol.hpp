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

	virtual void LinkFailure(int u, int v) = 0;
	virtual void DestinationAnnouncement(int t, int n, double period,
					     int max) = 0;
};

} // namespace Protocol
#endif
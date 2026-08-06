#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <limits>
#include <unordered_map>

#include "graph/graph.hpp"
#include "scheduler/calendar.hpp"
#include "scheduler/rng.hpp"
#include "utils/stats/data.hpp"

namespace Protocol
{
template <class A, class R, class Msg = R> struct IProtocol
{
	Graphs::Graph<A>     &graph;
	scheduler::Calendar   cal;
	utils::stats::Data<R> data;

	explicit IProtocol(Graphs::Graph<A> &graph_)
	    : graph(graph_)
	    , data(graph_.V)
	{
	}

	virtual ~IProtocol() = default;

	virtual void MsgReception(int u, int v, Msg msg) = 0;

	virtual void LinkFailure(int u, int v) = 0;

	virtual void process()
	{
	}

	void Run()
	{
		cal.run([this]() {
			process();
		});
	}

	void Send(int t, Msg msg)
	{
		for (auto &[r, edge] : graph.adjListIn[t])
		{
			auto &[delay, active, attr] = edge;

			if (!active)
				continue;

			double arrival = ScheduleTime(delay, r, t);

			cal.schedule(arrival, [this, r, t, msg]() mutable {
				this->MsgReception(r, t, msg);
			});
			data.recordMessage(t, cal.now());
		}
	}

    private:
	static constexpr double                   e = 0.01;
	std::unordered_map<std::uint64_t, double> prev_time;

	static std::uint64_t link_key(int from, int to)
	{
		return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(from)) << 32) |
		       static_cast<std::uint32_t>(to);
	}

	double ScheduleTime(double delay, int u, int v)
	{
		double arrival = cal.now() + rng_uniform(delay, 1.1 * delay);

		auto key            = link_key(v, u);
		auto [it, inserted] = prev_time.try_emplace(key, -std::numeric_limits<double>::infinity());
		if (arrival <= it->second)
		{
			arrival = it->second + e;
		}
		it->second = arrival;

		return arrival;
	}
};
} // namespace Protocol
#endif

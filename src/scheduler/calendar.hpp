#ifndef CALENDAR_HPP
#define CALENDAR_HPP

#include <functional>
#include <queue>
#include <utility>
#include <vector>

#include "event.hpp"

namespace scheduler
{
class Calendar
{
    public:
	Calendar() = default;

	template <class F>
	void schedule(double time, F f)
	{
		q.emplace(time, next_seq++, std::function<void()>(std::move(f)));
	}

	[[nodiscard]]
	bool empty() const
	{
		return q.empty();
	}

	[[nodiscard]]
	double now() const
	{
		return current_time;
	}

	void clear()
	{
		q = std::priority_queue<Event, std::vector<Event>, EventCompare>();
	}

	[[nodiscard]]
	std::uint64_t getSeq() const
	{
		return next_seq;
	}

	[[nodiscard]]
	double getTopTime() const
	{
		return q.top().time;
	}

	void step()
	{
		Event ev = std::move(const_cast<Event &>(q.top()));
		q.pop();
		current_time = ev.time;
		ev();
	}

	template <class F>
	void run(F process)
	{
		while (!q.empty())
		{
			const double snapshot = q.top().time;
			while (!q.empty() && q.top().time == snapshot)
			{
				step();
			}
			process();
		}
	}

    private:
	std::priority_queue<Event, std::vector<Event>, EventCompare> q;
	std::uint64_t                                                next_seq     = 0;
	double                                                       current_time = 0.0;
};

} // namespace scheduler
#endif

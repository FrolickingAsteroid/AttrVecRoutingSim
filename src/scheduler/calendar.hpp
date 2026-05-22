#ifndef CALENDAR_HPP
#define CALENDAR_HPP

#include "event.hpp"
#include <functional>
#include <queue>
#include <utility>
#include <vector>

namespace scheduler
{
class Calendar {
    public:
	Calendar() = default;

	template <class F> void schedule(double time, F f)
	{
		q.emplace(time, next_seq++,
			  std::function<void()>(std::move(f)));
	}

	bool empty() const
	{
		return q.empty();
	}

	double now() const
	{
		return current_time;
	}

	std::uint64_t getSeq() const
	{
		return next_seq;
	}

	void run()
	{
		while (!q.empty()) {
			Event ev = std::move(const_cast<Event &>(q.top()));
			q.pop();

			current_time = ev.time;

			ev();
		}
	}

    private:
	std::priority_queue<Event, std::vector<Event>, EventCompare> q;
	std::uint64_t next_seq = 0;
	double current_time = 0.0;
};
} // namespace scheduler
#endif // CALENDAR_HPP

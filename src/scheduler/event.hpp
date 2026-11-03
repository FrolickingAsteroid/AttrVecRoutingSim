#ifndef EVENT_HPP
#define EVENT_HPP

#include <cstdint>
#include <functional>
#include <utility>

struct Event
{
	double                time;
	std::uint64_t         seq;
	std::function<void()> e;

	Event(double time_, std::uint64_t seq_, std::function<void()> e_)
	    : time(time_)
	    , seq(seq_)
	    , e(std::move(e_))
	{
	}

	void operator()() const
	{
		e();
	}
};

struct EventCompare
{
	bool operator()(const Event &lhs, const Event &rhs) const
	{
		// std::priority_queue is a max-heap.
		// Return true when lhs has lower priority than rhs.
		if (lhs.time != rhs.time)
		{
			return lhs.time > rhs.time;
		}

		return lhs.seq > rhs.seq;
	}
};

#endif

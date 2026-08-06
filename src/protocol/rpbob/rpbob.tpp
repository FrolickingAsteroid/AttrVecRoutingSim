#ifndef PROTOCOL_TPP
#define PROTOCOL_TPP

#include "protocol/rpbob/rpbob.hpp"
#include "utils/ui/logger.hpp"

using namespace RoutingAlgebras;

namespace Protocol::Rpbob
{
template <class RA> void RPBOB<RA>::process()
{
	for (int u = 0; u < graph.V; ++u)
	{
		auto it   = data.finalValue.find(u);
		path best = s[u].GetBestAttr();

		if (best.attr == Ord::Null())
		{
			continue;
		}

		if (it == data.finalValue.end() || best.attr.a != it->second.attr.a)
		{
			data.recordChange(u, cal.now(), best);
		}
	}
}

template <class RA> void RPBOB<RA>::LinkFailure(int u, int v)
{
	data.log = true;
	data.setOffset(cal.now());

	graph.deactivate(u, v);

	LOG_DEBUG << "Time of Failure: " << cal.now() << "\n";

	if (cn)
	{
		for (auto [node, link] : {
			 std::tuple{u, std::pair{u, v}},
                         std::tuple{v, std::pair{v, u}}
                })
		{
			auto removed = s[node].FailureNotif(link);
			if (!removed.empty())
			{
				s[node].failed_link = link;
				for (const auto &current : removed)
				{
					s[u].UpdateBestAttr();
					auto notif = std::make_tuple(current.getAttr().n, link, current);
					this->Send(node, Msg{notif});
				}
			}
		}
	}
}

template <class RA> void RPBOB<RA>::MsgReception(int u, int v, Msg msg)
{
	std::visit([&](auto &&m) {
		this->HandleMsg(u, v, m);
	}, msg);
}

template <class RA> void RPBOB<RA>::HandleMsg(int u, int v, notification notif)
{
	(void)v;

	get<2>(notif).addAsEntry(u);

	if (auto removed = s[u].FailureNotif(get<2>(notif)); !removed.empty())
	{
		s[u].failed_link = get<1>(notif);
		s[u].UpdateBestAttr();
		for (const auto &current : removed)
		{
			auto notif_ = std::make_tuple(current.getAttr().n, get<1>(notif), current);
			this->Send(u, Msg{notif_});
		}
	}
}

template <class RA> void RPBOB<RA>::HandleMsg(int u, int v, path p)
{
	if (std::get<1>(graph.adjListIn[u].at(v)) == false)
		return;

	if (s[u].failed_link.has_value() &&
	    s[u].ContainsFailedLink(p, s[u].failed_link.value().first, s[u].failed_link.value().second))
	{
		return;
	}

	path link(u, A(std::get<2>(graph.adjListOut[u].at(v))));
	path extended = link + p;

	std::optional<path> changed = s[u].CompareSetAttr(extended);

	if (changed.has_value())
	{
		this->Send(u, Msg{extended});
		converged();
	}
}

template <class RA> void RPBOB<RA>::DestinationAnnouncement(int t, int n, double period, std::optional<int> max)
{
	s[t].SetOrigin(n, t);
	auto o = s[t].GetBestAttr();

	this->Send(t, Msg{o});

	n += 1;
	double d = period * n;

	const bool should_continue = max.has_value() ? (n < *max) : true;

	if (should_continue)
	{
		cal.schedule(d, [t, n, period, max, this]() mutable {
			this->DestinationAnnouncement(t, n, period, max);
		});
	}
}
} // namespace Protocol::Rpbob

#endif

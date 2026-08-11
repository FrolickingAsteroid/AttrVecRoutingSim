#ifndef PROTOCOL_TPP
#define PROTOCOL_TPP

#include "protocol/rpbob/rpbob.hpp"
#include "utils/ui/logger.hpp"

using namespace RoutingAlgebras;

namespace Protocol::Rpbob
{
template <class RA>
void RPBOB<RA>::process()
{
	for (int u = 0; u < graph.V; ++u)
	{
		auto it  = data.finalValue.find(u);
		auto opt = s[u].GetBestAttr();

		if (opt.empty())
		{
			continue;
		}

		if (it == data.finalValue.end() || !CompareOpt(opt, it->second))
		{
			data.recordChange(u, cal.now(), opt);
		}
	}
}

template <class RA>
void RPBOB<RA>::LinkFailure(int u, int v)
{
	data.log = true;
	data.setOffset(cal.now());

	graph.deactivate(u, v);

	LOG_DEBUG << s[34];
	LOG_DEBUG << "change count: " << data.changeCounts.at(27);
	LOG_DEBUG << cal.now() << "\n";

	LOG_DEBUG << s[38];
	LOG_DEBUG << "change count: " << data.changeCounts.at(38);
	LOG_DEBUG << cal.now() << "\n";

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
				if (s[node].UpdateBestAttr())
				{
					data.recordChange(node, cal.now(), s[node].GetBestAttr());
				}

				const auto &max =
				    *std::max_element(removed.begin(), removed.end(), [](const auto &a, const auto &b) {
					return a.attr.a < b.attr.a;
				});

				auto notif = std::make_tuple(max.attr.a, link);
				this->Send(node, Msg{notif});
			}
		}
	}
}

template <class RA>
void RPBOB<RA>::MsgReception(int u, int v, Msg msg)
{
	std::visit([&](auto &&m) {
		this->HandleMsg(u, v, m);
	}, msg);
}

template <class RA>
void RPBOB<RA>::HandleMsg(int u, int v, notification notif)
{
	(void)v;

	if (auto removed = s[u].FailureNotif(get<1>(notif)); !removed.empty())
	{
		s[u].failed_link = get<1>(notif);
		if (s[u].UpdateBestAttr())
		{
			data.recordChange(u, cal.now(), s[u].GetBestAttr());
		}
		this->Send(u, Msg{notif});
	}
}

template <class RA>
void RPBOB<RA>::HandleMsg(int u, int v, path p)
{
	if (u == 45)
	{
		int a = 1;
	}
	if (!graph.edgeState(u, v) || s[u].ContainsFailedLink(p))
	{
		return;
	}

	A edgeAttr(0, graph.edgeWeight(u, v), u);

	auto                extended = p + edgeAttr;
	std::optional<path> changed  = s[u].CompareSetAttr(extended);

	LOG_DEBUG << s[u];
	LOG_DEBUG << "change count: " << data.changeCounts.at(u);
	LOG_DEBUG << cal.now() << "\n";

	if (changed.has_value())
	{
		this->Send(u, Msg{extended});
		converged();
	}
}

template <class RA>
void RPBOB<RA>::DestinationAnnouncement(int t, int n, double period, std::optional<int> max)
{

	s[t].SetOrigin(n, t);
	for (auto o = s[t].GetBestAttr(); auto current : o)
	{
		this->Send(t, Msg{current});
	}

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

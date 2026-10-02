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
		auto opt = s[u].GetOpt();

		if (opt.empty())
		{
			// node has yet to register a route
			if (it == data.finalValue.end())
			{
				continue;
			}

			// had a route and lost it after failure
			opt = {RpbobAttr(RA::Null(-1))};
		}

		if (it == data.finalValue.end() || !utils::helpers::CompareOpt<RpbobAttr>(opt, it->second))
		{
			data.recordChange(u, cal.now(), opt);
		}
	}
}

// ---------------------------------------------------------------------------- *
//
// Link Failure Handling
//
// ---------------------------------------------------------------------------- *

template <class RA>
void RPBOB<RA>::NodeWithdraw(int u)
{
	data.log = true;
	data.setOffset(cal.now());

	graph.deactivate(u);

	s[u].ClearOpt();
}

template <class RA>
void RPBOB<RA>::LinkFailure(int u, int v)
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
				if (s[node].UpdateOpt())
				{
					data.recordChange(node, cal.now(), s[node].GetOpt());
				}

				const auto &max =
				    *std::max_element(removed.begin(), removed.end(), [](const auto &a, const auto &b) {
					return a.attr.getSeqNum() < b.attr.getSeqNum();
				});

				auto notif = std::make_tuple(max.attr.getSeqNum(), link);
				this->Send(node, Msg{notif});
			}
		}
	}
}

// ---------------------------------------------------------------------------- *
//
// Message Processing
//
// ---------------------------------------------------------------------------- *

template <class RA>
void RPBOB<RA>::MsgReception(int u, int v, Msg msg)
{
	std::visit([&](auto &&m) {
		this->HandleMsg(u, v, m);
	}, msg);
}

template <class RA>
void RPBOB<RA>::HandleMsg(int u, int v, Notification notif)
{
	(void)v;
	auto f_link = get<1>(notif);

	if (auto removed = s[u].FailureNotif(f_link); !removed.empty())
	{
		s[u].failed_link = f_link;
		if (s[u].UpdateOpt())
		{
			data.recordChange(u, cal.now(), s[u].GetOpt());
		}
		this->Send(u, Msg{notif});
		if (data.log == true)
		{
			converged();
		}
	}
}

template <class RA>
void RPBOB<RA>::HandleMsg(int u, int v, RpbobAttr p)
{
	if (!graph.edgeState(v, u) || s[u].ContainsFailedLink(p))
	{
		return;
	}

	typename RA::Attr edgeAttr(0, graph.edgeWeight(u, v).buildLinkAttr(u)); // (0, l_attr, u)
	auto              extended = p + edgeAttr;                              // (n, l_attr (ext) r_attr, uP)

	std::optional<RpbobAttr> changed = s[u].CompareWind(extended);

	LOG_DEBUG << "Time: " << cal.now();
	LOG_DEBUG << "node: " << u << "\nextended: " << extended;

	if (changed.has_value())
	{
		this->Send(u, Msg{extended});
		if (data.log == true)
		{
			converged();
		}
	}

	LOG_DEBUG << s[u] << "\n";
}

template <class RA>
void RPBOB<RA>::DestinationAnnouncement(int t, int n, double period, std::optional<int> max)
{

	s[t].SetOrigin(n, t);

	// destination has a single opt attr: (n, e, t)
	auto o = s[t].GetOpt().front();
	this->Send(t, Msg{o});

	n += 1;
	double d = period * (n - 1);

	const bool should_continue = max.has_value() ? (n <= *max) : true;

	if (should_continue)
	{
		cal.schedule(d, [t, n, period, max, this]() mutable {
			this->DestinationAnnouncement(t, n, period, max);
		});
	}
}
} // namespace Protocol::Rpbob

#endif

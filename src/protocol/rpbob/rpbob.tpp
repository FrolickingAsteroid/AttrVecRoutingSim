#ifndef PROTOCOL_TPP
#define PROTOCOL_TPP

#include "protocol/rpbob/rpbob.hpp"

using namespace RoutingAlgebras;

namespace Protocol::Rpbob
{

template <class RA> void RPBOB<RA>::process()
{
	for (int u = 0; u < graph.V; ++u) {
		auto	it   = data.finalValue.find(u);
		Path<A> best = s[u].GetBestAttr();

		if (best.attr == Ord::Null()) {
			continue;
		}

		if (it == data.finalValue.end() || best.attr.a != it->second.attr.a) {
			data.recordChange(u, cal.now(), best);
		}
	}
}

template <class RA> void RPBOB<RA>::LinkFailure(int u, int v)
{
	data.log = true;
	data.setOffset(cal.now());

	graph.deactivate(u, v);
	//graph.removeEdge(v, u);

	LOG_DEBUG << "Time of Failure: " << cal.now() << "\n";

	// auto notif = std::make_tuple(s[u].GetBestAttr().attr.n, u, v);

	//if (s[u].FailureNotif(notif)) {
	//	this->Send(u, Msg{ notif });
	//}
}

template <class RA> void RPBOB<RA>::MsgReception(int u, int v, Msg msg)
{
	std::visit([&](auto &&m) { this->HandleMsg(u, v, m); }, msg);
}

template <class RA>
void RPBOB<RA>::HandleMsg(int u, int v, std::tuple<int, int, int> notif)
{
	(void)v;
	if (s[u].FailureNotif(notif)) {
		this->Send(u, Msg{ notif });
	}
}

template <class RA> void RPBOB<RA>::HandleMsg(int u, int v, Path<A> p)
{
	if (std::get<1>(graph.adjListIn[u].at(v)) == false)
		return;

	Ext extension;

	Path<A> old_opt_attr = s[u].GetBestAttr();
	A	lw(std::get<2>(graph.adjListOut[u].at(v))); // Fetch link attr
	A	candidate = extension(lw, p.getAttr());

	p.addAsEntry(u);
	p.addAttr(candidate);

	std::optional<Path<A>> changed = s[u].CompareSetAttr(p);

	if (changed.has_value()) {
		this->Send(u, Msg{ p });
		//converged();
	}
}

template <class RA>
void RPBOB<RA>::DestinationAnnouncement(int t, int n, double period, int max)
{
	s[t].SetOrigin(n, t);
	auto o = s[t].GetBestAttr();

	this->Send(t, Msg{ o });

	n += 1;

	double d = period * n;
	if (n < max)
		cal.schedule(d, [t, n, period, max, this]() mutable {
			this->DestinationAnnouncement(t, n, period, max);
		});
}
}

#endif

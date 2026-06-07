#ifndef PROTOCOL_TPP
#define PROTOCOL_TPP

#include "protocol/rpbob/rpbob.hpp"

using namespace RoutingAlgebras;

namespace Protocol::Rpbob
{
template <class RA> void RPBOB<RA>::MsgReception(int u, int v, Path<A> p)
{
	Ext extension;

	Path<A> old_opt_attr = s[u].GetBestAttr();
	A	lw(std::get<2>(graph.adjListOut[u].at(v))); // Fetch link attr
	A	candidate = extension(lw, p.getAttr());

	p.addAsEntry(u);
	p.addAttr(candidate);

	std::optional<Path<A>> changed = s[u].CompareSetAttr(p);

	if (changed.has_value()) {
		this->Send(u, p);
		if (Path<A> new_opt_attr = s[u].GetBestAttr();
		    old_opt_attr.attr.a != new_opt_attr.attr.a) {
			data.recordChange(u, cal.now(), new_opt_attr);
		}
	}
}

template <class RA> void RPBOB<RA>::DestinationAnnouncement(int t, int n, double period, int max)
{
	Path<A> p(t, Ord::Neutral(n));

	this->Send(t, p);

	// schedule next routing computation
	n += 1;
	double d = period * n;
	if (n < max) {
		cal.schedule(d, [t, n, period, max, this]() mutable {
			this->DestinationAnnouncement(t, n, period, max);
		});
	}
}
}

#endif

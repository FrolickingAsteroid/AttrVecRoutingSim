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
			/*
			if (u == 0) {
				std::cout << "[";
				for (int node : best.path)
					std::cout << node << " ";
				std::cout << "] attr: " << best.attr.a << " seq: " << best.attr.n
					  << " cal: " << cal.now() << std::endl;
			}
			*/
			data.recordChange(u, cal.now(), best);
		}
	}
}

template <class RA> void RPBOB<RA>::LinkFailure(int u, int v)
{
	data.log = true;
	data.setOffset(cal.now());

	graph.removeEdge(u, v);
	graph.removeEdge(v, u);
}

template <class RA> void RPBOB<RA>::MsgReception(int u, int v, Path<A> p)
{
	if (graph.adjListOut[u].find(v) == graph.adjListOut[u].end())
		return;

	Ext extension;

	Path<A> old_opt_attr = s[u].GetBestAttr();
	A	lw(std::get<2>(graph.adjListOut[u].at(v))); // Fetch link attr
	A	candidate = extension(lw, p.getAttr());

	p.addAsEntry(u);
	p.addAttr(candidate);

	std::optional<Path<A>> changed = s[u].CompareSetAttr(p);

	if (changed.has_value()) {
		this->Send(u, p);
		converged();
	}
}

template <class RA> void RPBOB<RA>::DestinationAnnouncement(int t, int n, double period, int max)
{
	s[t].SetOrigin(n, t);
	auto o = s[t].GetBestAttr();

	this->Send(t, o);

	n += 1;

	double d = period * n;
	if (n < max)
		cal.schedule(d, [t, n, period, max, this]() mutable {
			this->DestinationAnnouncement(t, n, period, max);
		});
}
}

#endif

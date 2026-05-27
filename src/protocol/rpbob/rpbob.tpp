#ifndef PROTOCOL_TPP
#define PROTOCOL_TPP

#include "protocol/rpbob/rpbob.hpp"

namespace Protocol::Rpbob
{
template <class A, class Ord, class Ext>
void RPBOB<A, Ord, Ext>::MsgReception(int u, int v, path p)
{
	Ext extension;

	path old_opt_attr = s[u].GetBestAttr();
	seq_attr lw(std::get<2>(graph.adjListOut[u].at(v))); // Fetch link attr
	seq_attr candidate = extension(lw, p.getAttr());

	p.addAsEntry(u);
	p.addAttr(candidate);

	std::optional<path> changed = s[u].CompareSetAttr(p);

	if (changed.has_value()) {
		data.record(cal.now(), u, s[u].GetBestAttr(),
			    s[u].GetSetAttr());

		if (path new_opt_attr = s[u].GetBestAttr();
		    old_opt_attr.attribute.a != new_opt_attr.attribute.a) {
			data.record(cal.now(), u, new_opt_attr);
		}

		auto old_seq = cal.getSeq();
		this->Send(u,*changed);
		auto new_seq = cal.getSeq();
		data.record(cal.now(), u, "propagate", new_seq - old_seq);
	}
}

template <class A, class Ord, class Ext>
void RPBOB<A, Ord, Ext>::DestinationAnnouncement(int t, int n, double period,
						 int max)
{
	path p(t, Ord::Neutral(n));

	auto old_seq = cal.getSeq();
	this->Send(t, p);
	auto new_seq = cal.getSeq();
	data.record(cal.now(), t, "announcement", new_seq - old_seq);

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

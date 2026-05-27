#ifndef RPBOB_HPP
#define RPBOB_HPP

#include "protocol/protocol.hpp"
#include "protocol/rpbob/state/path-segment.hpp"
#include "protocol/rpbob/state/seq-window.hpp"
#include <vector>

namespace Protocol::Rpbob
{
template <class A, class Ord, class Ext> struct RPBOB : IProtocol<A, Path<A> > {
	using Base = IProtocol<A, Path<A> >;
	using Base::graph;
	using Base::cal;
	using Base::data;

	using path = Path<A>;
	using window = SequenceWindow<A, Ord>;
	using seq_attr = RoutingAlgebras::SeqAttr<A>;

	std::vector<window> s;

	explicit RPBOB(Graphs::Graph<A> &graph_, std::size_t N_)
		: Base(graph_)
		, s(graph_.V, window(N_))
	{
	}

	void MsgReception(int u, int v, path p) override;
	void DestinationAnnouncement(int t, int n, double period,
				     int max) override;
};
} // namespace Protocol::Rpbob

#include "protocol/rpbob/rpbob.tpp"

#endif

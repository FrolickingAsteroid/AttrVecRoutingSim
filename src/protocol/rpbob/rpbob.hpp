#ifndef RPBOB_HPP
#define RPBOB_HPP

#include "protocol/protocol.hpp"
#include "protocol/rpbob/state/path-segment.hpp"
#include "protocol/rpbob/state/seq-window.hpp"
#include <vector>

namespace Protocol::Rpbob
{
template <class RA> struct RPBOB : IProtocol<typename RA::BaseAttr, Path<typename RA::Attr>> {
	using Base = IProtocol<typename RA::BaseAttr, Path<typename RA::Attr>>;
	using Base::graph;
	using Base::cal;
	using Base::data;

	using A = RA::Attr;
	using Ord = RA::Ord;
	using Ext = RA::Ext;

	std::vector<SequenceWindow<A, Ord>> s;

	explicit RPBOB(Graph<typename RA::BaseAttr> &graph_, std::size_t N_)
		: Base(graph_)
		, s(graph_.V, SequenceWindow<A, Ord>(N_))
	{
	}

	void MsgReception(int u, int v, Path<A> p) override;
	void DestinationAnnouncement(int t, int n, double period, int max) override;
};
}

#include "protocol/rpbob/rpbob.tpp"

#endif

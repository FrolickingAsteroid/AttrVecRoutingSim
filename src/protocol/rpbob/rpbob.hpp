#ifndef RPBOB_HPP
#define RPBOB_HPP

#include <vector>

#include "protocol/protocol.hpp"
#include "protocol/rpbob/state/seq-window.hpp"
#include "protocol/rpbob/state/path-segment.hpp"

namespace Protocol::Rpbob
{
template <class RA> struct RPBOB : IProtocol<typename RA::BaseAttr, Path<typename RA::Attr>> {
	using Base = IProtocol<typename RA::BaseAttr, Path<typename RA::Attr>>;

	using Base::graph;
	using Base::cal;
	using Base::data;

	using A	  = RA::Attr;
	using Ord = RA::Ord;
	using Ext = RA::Ext;

	std::vector<SequenceWindow<A, Ord>> s;

	explicit RPBOB(Graph<typename RA::BaseAttr> &graph_, std::size_t N_)
		: Base(graph_)
		, s(graph_.V, SequenceWindow<A, Ord>(N_))
	{
	}

	explicit RPBOB(Graph<typename RA::BaseAttr> &graph_, std::vector<std::size_t> Ns)
		: Base(graph_)
	{
		s.reserve(graph_.V);

		for (std::size_t n : Ns)
			s.emplace_back(n);
	}

	void MsgReception(int u, int v, Path<A> p) override;

	void DestinationAnnouncement(int t, int n, double period, int max);

	void LinkFailure(int u, int v) override;

	void process() override;

	std::function<bool(int)> nodeConverged = nullptr;
	std::function<void()>	 converged     = nullptr;
};
}

#include "protocol/rpbob/rpbob.tpp"

#endif

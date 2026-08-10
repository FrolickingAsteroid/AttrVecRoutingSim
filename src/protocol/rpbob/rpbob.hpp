#ifndef RPBOB_HPP
#define RPBOB_HPP

#include <functional>
#include <tuple>
#include <variant>
#include <vector>

#include "protocol/protocol.hpp"
#include "protocol/rpbob/state/path-segment.hpp"
#include "protocol/rpbob/state/seq-window.hpp"

namespace Protocol::Rpbob
{
template <class RA>
struct RPBOB : IProtocol<typename RA::BaseAttr, Path<RA>, std::variant<Path<RA>, std::tuple<int, std::pair<int, int>>>>
{
	// Type Aliases
	using BaseAttr = RA::BaseAttr;
	using A        = RA::Attr;
	using Ord      = RA::Ord;
	using Ext      = RA::Ext;

	using path         = Path<RA>;
	using notification = std::tuple<int, std::pair<int, int>>; //(n, bullet, uv)
	using Msg          = std::variant<path, notification>;

	using Base = IProtocol<BaseAttr, path, Msg>;

	using Base::cal;
	using Base::data;
	using Base::graph;

	// State
	std::vector<SequenceWindow<RA>> s;

	// Constructors
	explicit RPBOB(Graph<BaseAttr> &graph_, const std::vector<std::size_t> &Ns, bool cn_)
	    : Base(graph_)
	    , cn(cn_)
	{
		s.reserve(graph_.V);
		for (std::size_t n : Ns)
		{
			s.emplace_back(n);
		}
	}

	// Message Processing
	void MsgReception(int u, int v, Msg msg) override;

	void HandleMsg(int u, int v, notification notif);

	void HandleMsg(int u, int v, path p);

	void DestinationAnnouncement(int t, int n, double period, std::optional<int> max);

	void LinkFailure(int u, int v) override;

	void process() override;

	// Callbacks
	std::function<void()> converged = nullptr;

	bool cn = false;
};
} // namespace Protocol::Rpbob

#include "protocol/rpbob/rpbob.tpp"

#endif

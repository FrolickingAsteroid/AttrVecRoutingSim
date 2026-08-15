#ifndef RPBOB_HPP
#define RPBOB_HPP

#include <functional>
#include <limits>
#include <tuple>
#include <variant>
#include <vector>

#include "protocol/protocol.hpp"
#include "protocol/rpbob/state/rpbob-attribute.hpp"
#include "protocol/rpbob/state/seq-window.hpp"

namespace Protocol::Rpbob
{
template <class RA>
struct RPBOB : IProtocol<typename RA::Base::Attr, RPBOBAttribute<RA>,
			 std::variant<RPBOBAttribute<RA>, std::tuple<int, std::pair<int, int>>>>
{
	// Type Aliases
	using RpbobAttr    = RPBOBAttribute<RA>;
	using Notification = std::tuple<int, std::pair<int, int>>; //(n, bullet, uv), should be a subtype of RPBOBAttr
	using Msg          = std::variant<RpbobAttr, Notification>;

	using Base = IProtocol<typename RA::Base::Attr, RpbobAttr, Msg>;

	using Base::cal;
	using Base::data;
	using Base::graph;

	// State
	std::vector<SequenceWindow<RA>> s;

	// Constructors
	explicit RPBOB(Graphs::Graph<typename RA::Base::Attr> &graph_, const std::vector<int> &Ws_, const bool cn_)
	    : Base(graph_)
	    , cn(cn_)
	{
		s.reserve(graph_.V);

		constexpr int maxN = std::numeric_limits<int>::max();

		const std::vector<int> &Ws = Ws_.empty() ? std::vector<int>(graph_.V, maxN) : Ws_;

		for (int W : Ws)
		{
			s.emplace_back(W);
		}
	}

	// Message Processing
	void MsgReception(int u, int v, Msg msg) override;

	void HandleMsg(int u, int v, Notification notif);

	void HandleMsg(int u, int v, RpbobAttr p);

	void DestinationAnnouncement(int t, int n, double period, std::optional<int> max);

	void LinkFailure(int u, int v) override;

	void process() override;

	// Callbacks
	std::function<void()> converged = nullptr;

	// flag for failure notification
	bool cn = false;
};
} // namespace Protocol::Rpbob

#include "protocol/rpbob/rpbob.tpp"

#endif

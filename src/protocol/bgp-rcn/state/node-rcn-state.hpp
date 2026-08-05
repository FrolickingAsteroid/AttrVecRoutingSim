#ifndef RCN_NODE_STATE_HPP
#define RCN_NODE_STATE_HPP

#include "rcn-msg.hpp"

#include "protocol/bgp-rcn/state/bgp-rcn-attribute.hpp"
#include "protocol/bgp/state/node-state.hpp"

namespace Protocol::RCN
{
template <class A, class Ext, class Ord> struct RCNNodeState : BGP::BGPNodeState<RCNAttribute<A, Ext, Ord>>
{
	using attr = RCNAttribute<A, Ext, Ord>;
	using base = BGP::BGPNodeState<RCNAttribute<A, Ext, Ord>>;

	int                          t = 0;
	std::unordered_map<int, int> seq_num;

	const attr &GetElected()
	{
		if (!base::E.isInvalid())
		{
			base::E.ts.at(base::E.as_path.back()) = t;
		}
		return base::GetElected();
	}

	bool UpdateSeqTable(const RCNMsg<A, Ext, Ord> &learned)
	{
		// x in update.r.aspath, update.r.ts(x) > seqnum(v,x)
		for (const auto &[x, ts_x] : learned.route.ts)
		{
			auto it = seq_num.find(x);
			if (it == seq_num.end() || (ts_x > it->second))
			{
				seq_num[x] = ts_x;
			}
			// if update.r.ts(x) < seqnum(v,x) -> old update
			else if (ts_x < it->second)
			{
				return false;
			}
		}

		// x = update.rcn.c, update.rcn.ts(c) > seqnum(v,x)
		if (learned.RCN.has_value())
		{
			auto [x, ts_x] = *learned.RCN;
			auto it        = seq_num.find(x);

			if (it == seq_num.end() || ts_x > it->second)
			{
				seq_num[x] = ts_x;
			}
			if (x == BGP::BGPNodeState<RCNAttribute<A, Ext, Ord>>::succ)
			{
				++t;
			}
		}

		RmStaleRoutes();
		return true;
	}

	void RmStaleRoutes()
	{
		std::vector<int> to_remove;
		for (const auto &[v, a] : this->aTab)
		{
			bool stale = false;
			for (const auto &[x, ts_x] : a.ts)
			{
				auto seq = seq_num.find(x);
				if (seq != seq_num.end() && ts_x < seq->second)
				{
					stale = true;
					break;
				}
			}
			if (stale)
			{
				to_remove.push_back(v);
			}
		}
		for (int v : to_remove)
		{
			this->UpdateNeighbor(v, attr::Invalid());
		}
	}
};
} // namespace Protocol::RCN
#endif

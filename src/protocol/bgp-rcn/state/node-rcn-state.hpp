#ifndef RCN_NODE_STATE_HPP
#define RCN_NODE_STATE_HPP

#include "protocol/bgp-rcn/state/bgp-rcn-attribute.hpp"
#include "protocol/bgp/state/node-state.hpp"
#include "rcn-msg.hpp"

namespace Protocol::RCN
{
template <class RA>
struct RCNNodeState : BGP::BGPNodeState<RCNAttribute<RA>>
{
	using attr = RCNAttribute<RA>;
	using base = BGP::BGPNodeState<RCNAttribute<RA>>;

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

	bool UpdateSeqTable(const RCNMsg<RA> &learned)
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
			if (x == BGP::BGPNodeState<RCNAttribute<RA>>::succ)
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

#ifndef BGP_NODE_STATE_HPP
#define BGP_NODE_STATE_HPP

#include <unordered_map>

#include "protocol/bgp/state/bgp-attribute.hpp"

namespace Protocol::BGP
{
template <class RA, class AttrType = BGPAttribute<RA>>
struct BGPNodeState
{

	using path = AttrType;

	using A   = RA::Attr;
	using Ord = RA::Ord;

	// Minimum Route Advertisement Interval
	double MRAI = 0;

	// using Route = std::pair<path, int>; //(attribute, succ)

	std::unordered_map<int, path> aTab;
	path                          E;
	path                          Av;
	path                          O;
	int                           succ{-1};

	explicit BGPNodeState()
	    : E(path::Invalid())
	    , Av(path::Invalid())
	    , O(path::Invalid())
	{
	}

	void UpdateNeighbor(int v, const path &learned)
	{
		aTab[v] = learned;
	}

	void RemoveNeighbor(int v)
	{
		aTab.erase(v);
	}

	void SetOrigin(int asn)
	{
		O = path::Neutral(asn);
	}

	bool ProcessBest(int v);

	bool ProcessBest();

	const path &GetElected()
	{
		return E;
	}

	const std::unordered_map<int, path> &GetAttrTable() const
	{
		return aTab;
	}

	bool HasRoute() const
	{
		return !E.isInvalid();
	}

	friend std::ostream &operator<<(std::ostream &os, const BGPNodeState &state)
	{
		os << "E: ";
		if (state.E.isInvalid())
		{
			os << "(none)";
		}
		else
		{
			os << state.E << " (succ=" << state.succ << ")";
		}
		os << "\n aTab: {\n";
		for (const auto &[neighbor, attr] : state.aTab)
		{
			os << neighbor << ": " << attr << "\n";
		}
		os << "}";
		return os;
	}

    protected:
	// Scan all of aTab and install the best into Av/succ.
	void fullScan();

	// Commit Av into E if they differ. Returns true if E changed.
	bool commitAv();
};
} // namespace Protocol::BGP

#include "protocol/bgp/state/node-state.tpp"
#endif
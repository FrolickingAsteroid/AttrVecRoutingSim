#ifndef BGP_NODE_STATE_HPP
#define BGP_NODE_STATE_HPP

#include <unordered_map>

#include "protocol/bgp/state/bgp-attribute.hpp"

namespace Protocol::BGP
{

// BGPNodeState holds all per-node routing state for a single destination.
//
// Fields:
//   aTab  — one learned attribute per out-neighbor
//   E     — currently elected (exported) attribute
//   Av    — best available attribute after last recomputation
//   O     — origin attribute; non-invalid only at the destination node
//   succ  — id of the neighbor whose attribute is currently elected, -1 if none
//
//
// BGPNodeState never calls the algebra directly; it only stores and compares
// BGPAttribute<A> values using operator< and operator!=.

template <class Attr> struct BGPNodeState
{
	using attr = Attr;

	std::unordered_map<int, attr> aTab;
	attr                          E;
	attr                          Av;
	attr                          O;
	int                           succ{-1};

	explicit BGPNodeState()
	    : E(attr::Invalid())
	    , Av(attr::Invalid())
	    , O(attr::Invalid())
	{
	}

	void UpdateNeighbor(int v, const attr &learned)
	{
		aTab[v] = learned;
	}

	void RemoveNeighbor(int v)
	{
		aTab.erase(v);
	}

	void SetOrigin(int asn)
	{
		O = attr::Neutral(asn);
	}

	// Recompute Av and succ from the current aTab and O.
	//
	// The three cases from processUpdateMessage are encoded here:
	//
	//   FastPath  — the incoming neighbor v already beats E: no full scan needed.
	//               Only valid when the caller knows v's new attribute is strictly
	//               better; pass v and the new attribute to skip the full scan.
	//
	//   FullScan  — used on withdrawal, link failure, or any case where the
	//               current succ may no longer be best. Pass v = -1.
	//
	// Returns true if E changed (i.e. the protocol layer must propagate).
	//
	// FastPath variant: caller provides the neighbor that just improved.
	bool ProcessBest(int v);

	// FullScan variant: used after RemoveNeighbor, SetOrigin, ClearOrigin,
	// or any event where the caller cannot guarantee who is now best.
	bool ProcessBest();

	// --- queries ---

	const attr &GetElected()
	{
		return E;
	}

	const std::unordered_map<int, attr> &GetAttrTable() const
	{
		return aTab;
	}

	bool HasRoute() const
	{
		return !E.isInvalid();
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
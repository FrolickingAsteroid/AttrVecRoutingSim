#ifndef BGP_ATTRIBUTE_HPP
#define BGP_ATTRIBUTE_HPP

#include <algorithm>
#include <iostream>
#include <vector>

namespace Protocol::BGP
{

static constexpr int MaxASPathLength = 35;

template <class A, class Ext, class Ord> struct BGPAttribute {
	A		 attribute;
	std::vector<int> as_path;

	BGPAttribute() = default;

	// Regular constructor: single origin ASN + metric attribute.
	// This is the normal way to build a fresh attribute at origination.
	BGPAttribute(int asn, A attr)
		: attribute(attr)
		, as_path({ asn })
	{
	}

	// Internal constructor used by operator+ and static factories:
	// takes a fully-formed path and a pre-built attribute.
	BGPAttribute(std::vector<int> path, A attr)
		: attribute(attr)
		, as_path(std::move(path))
	{
	}

	// --- static factories ---

	// Returns the neutral (origin) attribute for a destination node.
	// Replaces BGPAttribute(NEUTRAL, asn).
	// Ord::Neutral() must return an A that is the identity for extension.
	static BGPAttribute Neutral(int asn)
	{
		return BGPAttribute(asn, Ord::Neutral());
	}

	// Replaces BGPAttribute(INVALID).
	// Represented as an empty AS path — no valid origin.
	static BGPAttribute Invalid()
	{
		return BGPAttribute(std::vector<int>{}, Ord::Null());
	}

	// --- queries ---

	int length() const
	{
		return static_cast<int>(as_path.size());
	}

	bool isInvalid() const
	{
		return as_path.empty();
	}

	A getAttr() const
	{
		return attribute;
	}

	// --- mutators ---
	// Prepend a new AS hop to the front of the path.
	void addAsEntry(int asn)
	{
		as_path.insert(as_path.begin(), asn);
	}

	void addAttr(const A &attr)
	{
		attribute = attr;
	}

	// --- operators ---

	// Equality: both the metric AND the full AS path must match.
	// Used for detecting actual state change.
	bool operator==(const BGPAttribute &other) const
	{
		return attribute == other.attribute && as_path == other.as_path;
	}

	// Inequality: intentionally not the negation of ==.
	// Two attributes are "different" if their metric differs,
	// regardless of whether the AS paths are identical.
	bool operator!=(const BGPAttribute &other) const
	{
		return attribute != other.attribute || as_path != other.as_path;
	}

	bool operator<(const BGPAttribute &other) const
	{
		return attribute < other.attribute;

		// Preorder by AS path length as tiebreaker:
		// return attribute < other.attribute ||
		//        (attribute == other.attribute &&
		//         as_path.size() < other.as_path.size());

		// Total order, lexicographic tiebreak on second-to-last hop:
		// return attribute < other.attribute ||
		//        (attribute == other.attribute &&
		//         as_path.size() < other.as_path.size()) ||
		//        (attribute == other.attribute &&
		//         as_path.size() == other.as_path.size() &&
		//         as_path.size() > 1 &&
		//         as_path[as_path.size() - 2] < other.as_path[other.as_path.size() - 2]);
	}

	friend BGPAttribute operator+(const BGPAttribute &link, const BGPAttribute &a)
	{
		Ext ext;

		if (a.isInvalid()) {
			return BGPAttribute::Invalid();
		}

		// the path is already too long (> MaxASPathLength hops)
		if (static_cast<int>(a.as_path.size()) > MaxASPathLength) {
			return BGPAttribute::Invalid();
		}

		// the tail AS is already in the path (loop detection).
		if (std::find(a.as_path.begin(), a.as_path.end(), link.as_path[0]) !=
		    a.as_path.end()) {
			return BGPAttribute::Invalid();
		}

		std::vector<int> new_path(a.as_path);
		new_path.push_back(link.as_path[0]);

		return BGPAttribute(std::move(new_path),
				    ext(link.attribute, a.attribute));
	}

	friend std::ostream &operator<<(std::ostream &os, const BGPAttribute &a)
	{
		auto old_flags	   = os.flags();
		auto old_precision = os.precision();

		os << a.attribute;

		os.flags(old_flags);
		os.precision(old_precision);

		return os;
	}
};

}

#endif
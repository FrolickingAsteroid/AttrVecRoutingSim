#ifndef EQUALITY_HPP
#define EQUALITY_HPP

#include "routing-algebras/attributes/paths.hpp"
#include "routing-algebras/orders/comparison-vocab.hpp"

namespace RoutingAlgebras
{
template <class Node>
struct PathEquality
{
	// TODO: find a better way to represent the trivial path
	static Path<Node> neutral(Node t)
	{
		return Path<Node>(t);
	}
	inline static const Path<Node> null{};

	Comp operator()(const Path<Node> &p, const Path<Node> &q) const
	{
		if (p == q)
		{
			return Comp::Equal;
		}
		return Comp::Incomparable;
	}
};
} // namespace RoutingAlgebras
#endif

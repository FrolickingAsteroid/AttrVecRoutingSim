#ifndef PATHED_EXTENSION_HPP
#define PATHED_EXTENSION_HPP

#include "routing-algebras/attributes/pathed-attributes.hpp"
#include "routing-algebras/attributes/paths.hpp"
#include "routing-algebras/extensions/concatenation.hpp"

namespace RoutingAlgebras
{
template <class A, class Node, class ExtBase>
struct PathedExtension
{
	PathAttr<A, Node> operator()(const PathAttr<A, Node> &l, const PathAttr<A, Node> &r) const
	{
		ExtBase             ext;
		Concatenation<Node> prepend;

		Path<Node> p = prepend(l.getPath(), r.getPath());

		if (p.empty())
		{
			return PathAttr<A, Node>{A{}, p};
		}

		return PathAttr<A, Node>{ext(l.getAttr(), r.getAttr()), p};
	}
};
} // namespace RoutingAlgebras
#endif

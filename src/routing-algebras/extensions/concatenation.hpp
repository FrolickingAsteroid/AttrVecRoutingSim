#ifndef CONCATENATION_HPP
#define CONCATENATION_HPP

#include <algorithm>

#include "routing-algebras/attributes/paths.hpp"

namespace RoutingAlgebras
{
template <class Node>
struct Concatenation
{
	Path<Node> operator()(const Path<Node> &p, const Path<Node> &q) const
	{
		Path<Node> result = q;
		result.path.insert(result.path.end(), p.path.begin(), p.path.end());

		return result;
	}
};
} // namespace RoutingAlgebras
#endif

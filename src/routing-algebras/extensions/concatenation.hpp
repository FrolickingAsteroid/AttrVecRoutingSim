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
		// Extension assumes only simple paths, reject if q already appears
		// as a substring of p
		bool loop = std::search(p.path.begin(), p.path.end(), q.path.begin(), q.path.end()) != p.path.end();
		if (loop)
		{
			return {};
		}
		Path<Node> result = q;
		result.path.insert(result.path.end(), p.path.begin(), p.path.end());

		return result;
	}
};
} // namespace RoutingAlgebras
#endif

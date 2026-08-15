#ifndef SUFFIX_ORDER_HPP
#define SUFFIX_ORDER_HPP

#include <algorithm>
#include <limits>

#include "routing-algebras/attributes/paths.hpp"
#include "routing-algebras/orders/comparison-vocab.hpp"

namespace RoutingAlgebras
{
template <class Node>
struct SuffixOrder
{
	size_t m;

	explicit SuffixOrder(const size_t m_ = std::numeric_limits<std::size_t>::max())
	    : m(m_)
	{
	}

	Comp operator()(const Path<Node> &P, const Path<Node> &Q) const
	{
		if (P.size() != Q.size())
		{
			return P.size() < Q.size() ? Comp::Better : Comp::Worse;
		}

		// compare the last min(|P|, m) identifiers, back to front
		const std::size_t k = std::min({m, P.size(), Q.size()});

		auto p_begin = P.path.rbegin(); // reverse iterator, adding walks backwards
		auto p_end   = p_begin + static_cast<std::ptrdiff_t>(k);
		auto q_begin = Q.path.rbegin();
		auto q_end   = q_begin + static_cast<std::ptrdiff_t>(k);

		if (std::equal(p_begin, p_end, q_begin, q_end))
		{
			return Comp::Equal;
		}

		bool p_less = std::lexicographical_compare(p_begin, p_end, q_begin, q_end);
		bool q_less = std::lexicographical_compare(q_begin, q_end, p_begin, p_end);

		if (p_less)
		{
			return Comp::Better;
		}
		if (q_less)
		{
			return Comp::Worse;
		}
		return Comp::Incomparable;
	}
};
} // namespace RoutingAlgebras
#endif
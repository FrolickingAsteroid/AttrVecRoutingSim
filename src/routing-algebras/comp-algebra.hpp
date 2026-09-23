#ifndef COMP_ALGEBRA_HPP
#define COMP_ALGEBRA_HPP

#include <tuple>
#include <utility>

#include "routing-algebras/attributes/cartesian-product.hpp"

namespace RoutingAlgebras
{

template <template <class...> class CompOrd, class... Components>
struct CompAlgebra
{
	using Attr = CartProdAttr<typename Components::Attr...>;
	using Ord  = CompOrd<typename Components::Ord...>;

	struct Ext
	{
		Attr operator()(const Attr &a, const Attr &b) const
		{
			return extend(a, b, std::index_sequence_for<Components...>{});
		}

	    private:
		// automatically generate a list of indexes per attribute (at compile-time!)
		// for the entire length of the tuple of attributes
		template <std::size_t... Idx>
		static Attr extend(const Attr &a, const Attr &b, std::index_sequence<Idx...>)
		{
			// Ex.: tuple1 = (L1,W1) and tuple2 = (L2,W2)
			// Attr{ Components0::Ext{}(L1, L2),
			//       Components1::Ext{}(W1, W2)}
			return Attr{typename Components::Ext{}(std::get<Idx>(a), std::get<Idx>(b))...};
		}
	};

	template <typename Arg>
	static Attr Neutral(Arg &&arg)
	{
		// Sends args to each component algebra (either primitive or composite) and checks
		// weather their neutral element needs an init value.
		return Attr{Components::Neutral(std::forward<Arg>(arg))...};
	}

	static Attr Null()
	{
		return Attr{Components::Null()...};
	}
};
} // namespace RoutingAlgebras
#endif
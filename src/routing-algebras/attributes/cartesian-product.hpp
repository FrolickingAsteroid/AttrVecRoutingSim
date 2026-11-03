#ifndef CARTESIAN_PRODUCT_HPP
#define CARTESIAN_PRODUCT_HPP

#include <iosfwd>
#include <tuple>

#include "routing-algebras/attributes/paths.hpp"

namespace RoutingAlgebras
{

template <class... Attr>
struct CartProdAttr; // forward declaration

namespace PathFinder
{

template <class T>
struct FindPath;

template <class N>
struct FindPath<Path<N>>
{
	using Node = N;
	static const Path<N> &get(const Path<N> &p)
	{
		return p;
	}
	static void set(Path<N> &p, N x)
	{
		p = Path<N>{std::vector<N>{x}};
	}
};

template <class... A>
struct FindPath<CartProdAttr<A...>>
{
	static constexpr std::size_t last = sizeof...(A) - 1;
	using Tail                        = std::tuple_element_t<last, std::tuple<A...>>;
	using Node                        = typename FindPath<Tail>::Node;

	static const auto &get(const CartProdAttr<A...> &c)
	{
		return FindPath<Tail>::get(std::get<last>(c));
	}
	static void set(CartProdAttr<A...> &c, Node x)
	{
		FindPath<Tail>::set(std::get<last>(c), x);
	}
};
} // namespace PathFinder

template <class... Attr>
struct CartProdAttr : std::tuple<Attr...>
{
	// operators are already defined by the tuple, element-wise
	// struct inherits tuple constructor
	using Attrs = std::tuple<Attr...>;
	using Attrs::Attrs;

	// TODO: this most definitely must change in the future...
	// it assumes path routing algebras are always the last element of the
	// cartesian product, which is sorely not true.
	// See https://adamczapla.github.io/posts/implementing-tuple-find-a-constexpr-compatible-algorithm-for-heterogeneous-containers/
	// for a possible alternative
	auto getPath() const
	{
		return PathFinder::FindPath<CartProdAttr>::get(*this);
	}

	const CartProdAttr &getAttr() const
	{
		return *this;
	}

	template <class Node>
	CartProdAttr buildLinkAttr(Node x) const
	{
		CartProdAttr copy = *this;
		PathFinder::FindPath<CartProdAttr>::set(copy, x);
		return copy;
	}

	friend std::ostream &operator<<(std::ostream &os, const CartProdAttr &a)
	{
		os << "(";
		print(os, a, std::index_sequence_for<Attr...>{});
		return os << ")";
	}

	friend std::istream &operator>>(std::istream &is, CartProdAttr &a)
	{
		input(is, a, std::index_sequence_for<Attr...>{});
		return is;
	}

    private:
	template <std::size_t... I>
	static void print(std::ostream &os, const CartProdAttr &a, std::index_sequence<I...>)
	{
		std::size_t i = 0;
		// Another folded iteration. While there are attributes, keep printing
		((os << std::get<I>(a) << (++i < sizeof...(I) ? " " : "")), ...);
	}
	template <std::size_t... I>
	static void input(std::istream &is, CartProdAttr &a, std::index_sequence<I...>)
	{
		(is >> ... >> std::get<I>(a));
	}
};
} // namespace RoutingAlgebras
#endif

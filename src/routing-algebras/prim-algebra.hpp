#ifndef PRIM_ALGEBRA_HPP
#define PRIM_ALGEBRA_HPP
#include <complex>

namespace RoutingAlgebras
{
// TODO: Find a less ugly way to discern
// which orders need input to fetch their neutral component
template <typename Ord, typename Arg>
auto check(Arg &&arg)
{
	// constexpr has to be checked at compile time
	if constexpr (requires { Ord::neutral(arg); })
	{
		return Ord::neutral(arg);
	}
	else
	{
		return Ord::neutral;
	}
}

template <class Attr_, class Ext_, class Ord_>
struct PrimAlgebra
{
	using Attr = Attr_;
	using Ext  = Ext_;
	using Ord  = Ord_;

	template <typename Args>
	static Attr Neutral(Args arg)
	{
		return check<Ord>(arg);
	}
	static Attr Null()
	{
		return Ord::null;
	}
};

} // namespace RoutingAlgebras
#endif
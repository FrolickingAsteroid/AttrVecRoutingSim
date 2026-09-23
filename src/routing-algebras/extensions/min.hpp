#ifndef MIN_HPP
#define MIN_HPP

namespace RoutingAlgebras
{
template <class A>
struct Minimum
{
	A operator()(const A &l, const A &r) const
	{

		return std::min(l , r);
	}
};
} // namespace RoutingAlgebras

#endif

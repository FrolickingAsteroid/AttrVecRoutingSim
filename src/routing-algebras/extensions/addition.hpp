#ifndef LENGTHS_EXTENSION_H
#define LENGTHS_EXTENSION_H

namespace RoutingAlgebras
{
template <class A>
struct Addition
{
	A operator()(const A &l, const A &r) const
	{
		return l + r;
	}
};
} // namespace RoutingAlgebras

#endif

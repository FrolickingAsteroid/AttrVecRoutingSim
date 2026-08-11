#ifndef LENGTHS_EXTENSION_H
#define LENGTHS_EXTENSION_H

namespace RoutingAlgebras
{
typedef struct Addition
{
	L operator()(const L &l, const L &r) const
	{
		return L{l.getAttr() + r.getAttr()};
	}
} ADD;
} // namespace RoutingAlgebras

#endif

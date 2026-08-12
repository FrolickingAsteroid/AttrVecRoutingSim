#ifndef LENGTHS_EXTENSION_H
#define LENGTHS_EXTENSION_H

namespace RoutingAlgebras
{
typedef struct Addition
{
	int operator()(const int &l, const int &r) const
	{
		return l + r;
	}
} ADD;
} // namespace RoutingAlgebras

#endif

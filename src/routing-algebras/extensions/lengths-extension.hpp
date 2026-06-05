#ifndef LENGTHS_EXTENSION_H
#define LENGTHS_EXTENSION_H

namespace RoutingAlgebras
{

typedef struct LengthExtension {
	int operator()(int l, int r) const
	{
		int sum = l + r;
		return sum;
	}
} LE;
}

#endif

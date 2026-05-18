#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <fstream>
#include <vector>

namespace utils
{
template <class T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v)
{
	os << "[";
	for (std::size_t i = 0; i < v.size(); ++i) {
		if (i > 0)
			os << " ";
		os << v[i];
	}
	os << "]";
	return os;
}
}
#endif

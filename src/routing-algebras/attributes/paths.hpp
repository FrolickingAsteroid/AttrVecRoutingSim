#ifndef PATHS_HPP
#define PATHS_HPP

#include <ostream>
#include <vector>

namespace RoutingAlgebras
{
template <class Node>
struct Path
{
	std::vector<Node> path;

	Path() = default;

	Path(std::vector<Node> path_)
	    : path(std::move(path_))
	{
	}

	Path(Node n)
	    : path{std::move(n)}
	{
	}

	// ---------------------------------------------------------------------------- *
	//
	// Getters and Setters
	//
	// ---------------------------------------------------------------------------- *

	void setPath(std::vector<Node> path_)
	{
		path = std::move(path_);
	}

	const Path &getPath() const
	{
		return *this;
	}

	bool empty() const
	{
		return path.empty();
	}

	std::size_t size() const
	{
		return path.size();
	}

	const Node &back() const
	{
		return path.back();
	}

	const Node &front() const
	{
		return path.front();
	}

	// ---------------------------------------------------------------------------- *
	//
	// Operators
	//
	// ---------------------------------------------------------------------------- *

	const Node &operator[](std::size_t i) const
	{
		return path[i];
	}

	bool operator==(const Path &other) const
	{
		return path == other.path;
	}

	bool operator!=(const Path &other) const
	{
		return !(*this == other);
	}

	friend std::ostream &operator<<(std::ostream &os, const Path &p)
	{
		os << "[";
		for (size_t i = 0; i < p.path.size(); ++i)
		{
			os << p.path[i];
			if (i + 1 < p.path.size())
				os << " ";
		}
		return os << "]";
	}

	friend std::istream &operator>>(std::istream &is, Path &p)
	{
		p.path.clear();
		return is;
	}
};
} // namespace RoutingAlgebras
#endif

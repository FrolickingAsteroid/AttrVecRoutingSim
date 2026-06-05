#ifndef MP_PATH_SEGMENT_H
#define MP_PATH_SEGMENT_H

#include <vector>

namespace Protocol::Rpbob
{
template <class A> struct Path {
	A attr;
	std::vector<int> path;

	Path(int e, A a)
		: attr(a)
	{
		addAsEntry(e);
	}

	void addAsEntry(const int &entry)
	{
		path.insert(path.begin(), entry);
	}

	A getAttr()
	{
		return attr;
	}

	void addAttr(A &attr_)
	{
		attr = attr_;
	}

	bool operator==(const Path<A> &other) const
	{
		return attr == other.attr && path == other.path;
	}

	bool operator!=(const Path<A> &other) const
	{
		return !(*this == other);
	}

	friend std::ostream &operator<<(std::ostream &os, const Path<A> &p)
	{
		os << "(" << p.attr.n << ";" << p.attr.a << ")";
		return os;
	}
};
} // namespace Protocol::Rpbob

#endif

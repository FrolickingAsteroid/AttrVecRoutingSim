#ifndef MP_PATH_SEGMENT_H
#define MP_PATH_SEGMENT_H

#include <vector>

namespace Protocol::Rpbob
{
template <class RA> struct Path
{
	using A   = RA::Attr;
	using Ext = RA::Ext;
	using Ord = RA::Ord;

	A                attr;
	std::vector<int> path;

	Path() = default;

	Path(int e, A a)
	    : attr(a)
	{
		addAsEntry(e);
	}

	Path(std::vector<int> path_, A attr_)
	    : attr(attr_)
	    , path(std::move(path_))
	{
	}

	static Path Invalid()
	{
		return Path(std::vector<int>{}, Ord::Null());
	}

	bool isInvalid() const
	{
		return path.empty();
	}

	void addAsEntry(const int &entry)
	{
		path.insert(path.begin(), entry);
	}

	A getAttr() const
	{
		return attr;
	}

	void addAttr(A &attr_)
	{
		attr = attr_;
	}

	bool operator==(const Path &other) const
	{
		return attr == other.attr && path == other.path;
	}

	bool operator!=(const Path &other) const
	{
		return !(*this == other);
	}

	friend Path operator+(const Path &link, const Path &p)
	{
		auto loop = std::find(p.path.begin(), p.path.end(), link.path.front()) != p.path.end();
		if (loop)
		{
			return Invalid();
		}

		Ext  ext;
		Path extended(p.path, ext(link.attr, p.attr));

		extended.addAsEntry(link.path.front());
		return extended;
	}

	friend std::ostream &operator<<(std::ostream &os, const Path &p)
	{
		os << p.attr.a;
		return os;
	}
};
} // namespace Protocol::Rpbob

#endif

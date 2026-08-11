#ifndef MP_PATH_SEGMENT_H
#define MP_PATH_SEGMENT_H
namespace Protocol::Rpbob
{
template <class RA>
struct Path
{
	using A   = RA::Attr;
	using Ext = RA::Ext;
	using Ord = RA::Ord;

	A attr; //(n, attribute, path)

	Path() = default;

	Path(A attr_)
	    : attr(std::move(attr_))
	{
	}

	A getAttr() const
	{
		return attr;
	}

	static Path Invalid()
	{
		return Path(RA::Null());
	}
	bool isInvalid() const
	{
		auto p = attr.getAttr().getPath();
		return p.empty();
	}

	bool operator==(const Path &other) const
	{
		return attr == other.attr;
	}
	bool operator!=(const Path &other) const
	{
		return !(*this == other);
	}

	friend Path operator+(const Path &p, const A &link)
	{
		Ext ext;

		auto extended = ext(p.getAttr(), link);

		return Path(extended);
	}

	friend std::ostream &operator<<(std::ostream &os, const Path &p)
	{
		return os << p.attr.b;
	}
};

// path-segment.hpp
template <class RA>
std::ostream &operator<<(std::ostream &os, const std::vector<Path<RA>> &v)
{

	for (std::size_t i = 0; i < v.size(); ++i)
	{
		os << v[i];
		if (i + 1 < v.size())
			os << "; ";
	}
	return os;
}

template <class RA>
bool CompareOpt(const std::vector<Path<RA>> &l, const std::vector<Path<RA>> &r)
{
	if (l.size() != r.size())
	{
		return false;
	}
	for (const auto &l_ : l)
	{
		bool found = std::any_of(r.begin(), r.end(), [&](const Path<RA> &r_) {
			return l_.attr.b == r_.attr.b;
		});
		if (!found)
		{
			return false;
		}
	}
	return true;
}

template <class RA>
bool CompareOpt(const std::vector<Path<RA>> &l, const std::vector<typename RA::BaseAttr> &r)
{
	if (l.size() != r.size())
	{
		return false;
	}
	for (const auto &l_ : l)
	{
		bool found = std::any_of(r.begin(), r.end(), [&](const typename RA::BaseAttr &r_) {
			return l_.attr.b == r_;
		});
		if (!found)
		{
			return false;
		}
	}
	return true;
}
} // namespace Protocol::Rpbob
#endif
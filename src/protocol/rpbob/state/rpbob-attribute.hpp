#ifndef MP_PATH_SEGMENT_H
#define MP_PATH_SEGMENT_H

namespace Protocol::Rpbob
{
template <class RA>
struct RPBOBAttribute
{
	using A   = RA::Attr;
	using Ext = RA::Ext;
	using Ord = RA::Ord;

	A attr; //(n, attribute, path)

	RPBOBAttribute() = default;

	RPBOBAttribute(A attr_)
	    : attr(std::move(attr_))
	{
	}

	A getAttr() const
	{
		return attr;
	}

	bool isInvalid() const
	{
		auto p = attr.getAttr().getPath();
		return p.empty();
	}

	bool operator==(const RPBOBAttribute &other) const
	{
		return attr == other.attr;
	}
	bool operator!=(const RPBOBAttribute &other) const
	{
		return !(*this == other);
	}

	friend RPBOBAttribute operator+(const RPBOBAttribute &p, const A &link)
	{
		Ext ext;

		auto extended = ext(p.getAttr(), link);

		return RPBOBAttribute(extended);
	}

	friend std::ostream &operator<<(std::ostream &os, const RPBOBAttribute &p)
	{
		return os << p.attr;
	}

	friend std::ostream &operator<<(std::ostream &os, const std::vector<RPBOBAttribute<RA>> &v)
	{

		for (std::size_t i = 0; i < v.size(); ++i)
		{
			os << v[i];
			if (i + 1 < v.size())
				os << "; ";
		}
		return os;
	}
};
} // namespace Protocol::Rpbob
#endif
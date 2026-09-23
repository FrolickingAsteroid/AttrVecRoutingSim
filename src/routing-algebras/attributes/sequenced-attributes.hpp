#ifndef SEQUENCED_H
#define SEQUENCED_H

#include <ostream>
#include <tuple>
#include <utility>

namespace RoutingAlgebras
{
template <class A>
struct SeqAttr : std::tuple<int, A>
{
	// Publicly inheriting std::tuple<int, A>
	using Base = std::tuple<int, A>;

	SeqAttr() = default;

	// n becomes the sequence number, everything else forwards into A's constructor.
	// std::tuple's constructor takes one already-constructed value!!
	template <class... Args>
	SeqAttr(const int n_, Args &&...args)
	    : Base(n_, A(std::forward<Args>(args)...))
	{
	}

	// ---------------------------------------------------------------------------- *
	//
	// Getters and Setters
	//
	// ---------------------------------------------------------------------------- *

	void setSeqNum(const int a_)
	{
		std::get<0>(*this) = a_;
	}

	void setAttr(const A &b_)
	{
		std::get<1>(*this) = b_;
	}

	const int &getSeqNum() const
	{
		return std::get<0>(*this);
	}

	const A &getAttr() const
	{
		return std::get<1>(*this);
	}

	// bool empty() const
	// {
	// 	return std::get<1>(*this).empty();
	// }

	// ---------------------------------------------------------------------------- *
	//
	// Operators
	//
	// ---------------------------------------------------------------------------- *

	bool operator==(const SeqAttr &other) const
	{
		return static_cast<const Base &>(*this) == static_cast<const Base &>(other);
	}

	bool operator!=(const SeqAttr &other) const
	{
		return !(*this == other);
	}

	bool operator==(const A &other) const
	{
		return std::get<1>(*this) == other;
	}

	friend std::ostream &operator<<(std::ostream &os, const SeqAttr &attr)
	{
		os << "{" << std::get<0>(attr) << " " << std::get<1>(attr) << "}";
		return os;
	}
};
} // namespace RoutingAlgebras

#endif
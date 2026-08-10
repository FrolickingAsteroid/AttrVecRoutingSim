#ifndef SEQ_WINDOW_HPP
#define SEQ_WINDOW_HPP

#include <optional>
#include <vector>

#include "path-segment.hpp"
// #include "routing-algebras/extensions/seq-extension.hpp"
#include "routing-algebras/orders/sequenced-order.hpp"

namespace Protocol::Rpbob
{
template <class RA>
class SequenceWindow
{
	using A    = RA::Attr;
	using path = Path<RA>;

	using Ord     = RA::Ord;
	using BaseOrd = RA::BaseOrd;

    public:
	SequenceWindow(const std::size_t N_)
	    : W(N_)
	    , R(N_)
	{
	}

	void SetOrigin(int n, int u);

	bool UpdateBestAttr();

	std::vector<path> GetBestAttr() const;

	const std::vector<path> &GetSetAttr() const;

	std::optional<path> CompareSetAttr(const path &c);

	std::vector<path> FailureNotif(std::pair<int, int> link_notif);

	static bool ContainsFailedLink(const path &p, int u, int v);

	std::optional<std::pair<int, int>> failed_link = std::nullopt;

    private:
	std::size_t W;
	int         R;

	std::vector<path> wind;
	std::vector<path> opt;

	template <class Pred>
	std::vector<path> RemoveMatching(Pred pred);
};

template <class RA>
std::ostream &operator<<(std::ostream &os, const SequenceWindow<RA> &sw)
{
	const auto &best = sw.GetBestAttr();

	os << "opt: [";
	for (std::size_t i = 0; i < best.size(); ++i)
	{
		os << best[i].attr << " " << best[i].attr.b.getPath();
		if (i + 1 < best.size())
		{
			os << ",";
		}
	}
	os << "]";

	os << " wind [";
	const auto &set = sw.GetSetAttr();
	for (std::size_t i = 0; i < set.size(); ++i)
	{
		os << set[i].attr;
		if (i + 1 < set.size())
		{
			os << ",";
		}
	}
	os << "]";
	return os;
}
} // namespace Protocol::Rpbob

#include "protocol/rpbob/state/seq-window.tpp"

#endif

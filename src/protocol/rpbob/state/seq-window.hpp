#ifndef SEQ_WINDOW_HPP
#define SEQ_WINDOW_HPP

#include <optional>
#include <vector>

#include "path-segment.hpp"
#include "routing-algebras/extensions/seq-extension.hpp"
#include "routing-algebras/orders/sequenced-order.hpp"

namespace Protocol::Rpbob
{
template <class A, class Ord> class SequenceWindow
{
    public:
	using path = Path<A>;

	SequenceWindow(const std::size_t N_)
	    : N(N_)
	    , best_attr(path(-1, Ord::Null()))
	{
	}

	void SetOrigin(int n, int u);

	void UpdateBestAttr();

	const path &GetBestAttr() const;

	const std::vector<path> &GetSetAttr() const;

	std::optional<path> CompareSetAttr(const path &c);

	std::vector<path> FailureNotif(std::pair<int, int> link_notif);

	std::vector<path> FailureNotif(const path &path_notif);

	static bool ContainsFailedLink(const path &p, int u, int v);

	std::optional<std::pair<int, int>> failed_link = std::nullopt;

	std::size_t N;

    private:
	// std::size_t N;
	std::vector<path> set_seq_attr;
	path              best_attr;

	template <class Pred> std::vector<path> RemoveMatching(Pred pred);
};
} // namespace Protocol::Rpbob

#include "protocol/rpbob/state/seq-window.tpp"

#endif

#ifndef SEQ_WINDOW_HPP
#define SEQ_WINDOW_HPP

#include "path-segment.hpp"
#include "routing-algebras/extensions/seq-extension.hpp"
#include "routing-algebras/orders/sequenced-order.hpp"

#include <optional>
#include <vector>

namespace Protocol::Rpbob
{
template <class A, class Ord> class SequenceWindow {
    public:
	using path = Path<A>;

	SequenceWindow(const std::size_t N_)
		: N(N_)
		, best_attr(path(-1, Ord::Null()))
	{
	}

	void SetOrigin(int n, int u);

	const path &GetBestAttr() const;

	const std::vector<path> &GetSetAttr() const;

	std::optional<path> CompareSetAttr(const path &c);

	bool FailureNotif(std::tuple<int, int, int> notif);

    private:
	std::size_t	  N;
	std::vector<path> set_seq_attr;
	path		  best_attr;
};
}

#include "protocol/rpbob/state/seq-window.tpp"

#endif

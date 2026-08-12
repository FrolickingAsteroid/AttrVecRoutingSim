#ifndef SEQ_WINDOW_HPP
#define SEQ_WINDOW_HPP

#include <optional>
#include <vector>

#include "rpbob-attribute.hpp"

namespace Protocol::Rpbob
{
template <class RA>
class SequenceWindow
{
	using path = RPBOBAttribute<RA>;

	using A   = RA::Attr;
	using Ord = RA::Ord;

    public:
	SequenceWindow(const int N_)
	    : W(N_)
	    , R(N_)
	{
	}

	void SetOrigin(int n, int u);

	bool UpdateOpt();

	std::vector<path> GetOpt() const;

	const std::vector<path> &GetWind() const;

	std::optional<path> CompareWind(const path &c);

	std::vector<path> FailureNotif(std::pair<int, int> link_notif);

	static bool ContainsFailedLink(const path &p, int u, int v);

	bool ContainsFailedLink(const path &p) const;

	std::optional<std::pair<int, int>> failed_link = std::nullopt;

	friend std::ostream &operator<<(std::ostream &os, const SequenceWindow &sw)
	{
		const auto &best = sw.GetOpt();

		os << "W: " << sw.W << " R: " << sw.R << std::endl;
		os << "opt: [";
		for (std::size_t i = 0; i < best.size(); ++i)
		{
			os << best[i].attr;
			if (i + 1 < best.size())
			{
				os << ", ";
			}
		}
		os << "]";

		os << " wind: [";
		const auto &set = sw.GetWind();
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

    private:
	int W;
	int R;

	std::vector<path> wind;
	std::vector<path> opt;
};

} // namespace Protocol::Rpbob

#include "protocol/rpbob/state/seq-window.tpp"

#endif

#ifndef COMPARISON_HPP
#define COMPARISON_HPP

namespace RoutingAlgebras
{
enum class Comp
{
	Better,
	Equal,
	Worse,
	Incomparable
};

inline bool BetterOrEqual(Comp r)
{
	return r == Comp::Better || r == Comp::Equal;
}
} // namespace RoutingAlgebras
#endif

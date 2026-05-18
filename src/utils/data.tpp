#ifndef DATA_TPP
#define DATA_TPP

#include "utils/data.hpp"

namespace utils
{
template <class R>
void Registrar<R>::RegisterSet(const int dest, const Data<R> &data) const
{
	auto out = makeFile(std::to_string(dest), "set");

	out << "time,node,value,set\n";

	for (auto &e : data.entries) {
		out << e.time << "," << e.node << ","
		    << "\"" << e.value << "\",";
		if (e.set)
			out << "\"" << *e.set << "\"";
		out << "\n";
	}
}

template <class R>
void Registrar<R>::RegisterNodeStab(const int dest, const Data<R> &data)
{
	if (destination < 0) {
		WriteStab(dest, data, all_stab);
		return;
	}

	auto out = makeFile(std::to_string(dest), "stab");
	out << "dest,node,stab_time\n";
	WriteStab(dest, data, out);
}

template <class R>
void Registrar<R>::WriteStab(const int dest, const Data<R> &data,
			     std::ostream &out) const
{
	std::unordered_map<int, double> last_change;
	for (auto &e : data.entries) {
		if (e.set == std::nullopt) {
			last_change[e.node] = e.time;
		}
	}
	for (auto &[node, time] : last_change) {
		out << dest << "," << node << "," << time << "\n";
	}
}
}
#endif

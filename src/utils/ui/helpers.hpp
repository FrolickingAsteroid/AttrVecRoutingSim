#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <tabulate/tabulate.hpp>

namespace utils::ui
{

tabulate::Table::Row_t Row(std::initializer_list<std::string> cells)
{
	tabulate::Table::Row_t row;

	for (const auto &cell : cells)
		row.emplace_back(cell);

	return row;
}

namespace colors
{
constexpr const char *bold = "\033[1m";
constexpr const char *cyan = "\033[36m";
constexpr const char *yellow = "\033[33m";
constexpr const char *reset = "\033[0m";
}

std::string bold(const std::string &s)
{
	return std::string(colors::bold) + s + colors::reset;
}

std::string cyan(const std::string &s)
{
	return std::string(colors::cyan) + s + colors::reset;
}

std::string yellow(const std::string &s)
{
	return std::string(colors::yellow) + s + colors::reset;
}
}
#endif

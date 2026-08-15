#ifndef APPEARANCE_HPP
#define APPEARANCE_HPP

#include <sys/ioctl.h>
#include <tabulate/tabulate.hpp>

#include <cmath>
#include <iostream>

namespace utils::ui
{

inline void printProgressBar(std::size_t current, std::size_t total, int width = 40, const std::string &label = "")
{
	if (total == 0)
	{
		return;
	}

	double fraction = static_cast<double>(current) / static_cast<double>(total);
	int    filled   = static_cast<int>(fraction * width);

	std::printf("\r%s[", label.c_str());
	for (int i = 0; i < width; ++i)
	{
		std::putchar(i < filled ? '#' : ' ');
	}
	std::printf("] %3d%% (destination: %zu/%zu)", static_cast<int>(fraction * 100), current - 1, total - 1);
	std::fflush(stdout);

	if (current == total)
	{
		std::putchar('\n'); // finish the line once complete
	}
}
inline int terminalWidth()
{
	struct winsize w{};
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	int width = w.ws_col > 0 ? static_cast<int>(w.ws_col) : 80;
	return width % 2 != 0 ? width - 1 : width;
}

inline int numColumns(int col_width)
{
	return std::max(1, terminalWidth() / col_width);
}

inline int numRows(int total, int n_cols)
{
	return static_cast<int>(std::ceil(static_cast<double>(total) / n_cols));
}

inline int dataIndex(int row, int col, int n_rows)
{
	return col * n_rows + row;
}

inline tabulate::Table::Row_t Row(std::initializer_list<std::string> cells)
{
	tabulate::Table::Row_t row;

	for (const auto &cell : cells)
	{
		row.emplace_back(cell);
	}

	return row;
}

static std::string formatParam(double x)
{
	std::string s = std::to_string(x);
	return s.substr(0, s.find('.') + 2);
}

namespace colors
{
constexpr const char *bold   = "\033[1m";
constexpr const char *cyan   = "\033[36m";
constexpr const char *yellow = "\033[33m";
constexpr const char *reset  = "\033[0m";
constexpr const char *purple = "\033[35m";
} // namespace colors

inline std::string bold(const std::string &s)
{
	return std::string(colors::bold) + s + colors::reset;
}

inline std::string cyan(const std::string &s)
{
	return std::string(colors::cyan) + s + colors::reset;
}

inline std::string yellow(const std::string &s)
{
	return std::string(colors::yellow) + s + colors::reset;
}
inline std::string purple(const std::string &s)
{
	return std::string(colors::purple) + s + colors::reset;
}
inline void clear()
{
	std::cout << "\033[H\033[J";
}

} // namespace utils::ui
#endif

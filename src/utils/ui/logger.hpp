#ifndef LOGGER_H
#define LOGGER_H

#include <tabulate/tabulate.hpp>

#include <iostream>
#include <sstream>

namespace utils::ui
{

enum class LogLevel
{
	Debug   = 0,
	Info    = 1,
	Warning = 2,
	Silent  = 3
};

inline LogLevel global_level = LogLevel::Info;

inline void setInfo()
{
	global_level = LogLevel::Info;
}
inline void setVerbose()
{
	global_level = LogLevel::Debug;
}
inline void setSilent()
{
	global_level = LogLevel::Silent;
}

class LogLine
{
    public:
	LogLine(LogLevel level)
	    : level_(level)
	{
	}

	~LogLine()
	{
		if (level_ >= global_level)
			std::cout << ss_.str() << "\n";
	}

	template <typename T> LogLine &operator<<(const T &val)
	{
		ss_ << val;
		return *this;
	}

	LogLine &operator<<(tabulate::Table &table)
	{
		table.print(std::cout);
		return *this;
	}

    private:
	LogLevel           level_;
	std::ostringstream ss_;
};

} // namespace utils::ui

#define LOG_DEBUG utils::ui::LogLine(utils::ui::LogLevel::Debug)
#define LOG_INFO utils::ui::LogLine(utils::ui::LogLevel::Info)
#define LOG_WARNING utils::ui::LogLine(utils::ui::LogLevel::Warning)

#endif // LOGGER_H

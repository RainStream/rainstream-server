#pragma once

namespace rs
{
#define APP_NAME = "rainstream";

	class Logger
	{
	public:
		Logger(std::string prefix);

		void debug(const char* format, ...);

		void info(const char* format, ...);

		void warn(const char* format, ...);

		void error(const char* format, ...);

		void abort(const char* format, ...);

	private:
		std::string prefix;

	};

#define RS_ABORT(desc, ...) \
	do \
	{ \
		std::fprintf(stderr, "ABORT" desc , ##__VA_ARGS__); \
		std::fflush(stderr); \
		std::abort(); \
	} \
	while (false)

}

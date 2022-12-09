/**
 * Logger facility.
 *
 * This include file defines logging macros for source files (.cpp). Each
 * source file including Logger.h MUST define its own MSC_CLASS macro. Include
 * files (.h) MUST NOT include Logger.h.
 *
 * All the logging macros use the same format as printf().
 *
 * If the macro MSC_LOG_FILE_LINE is defied, all the logging macros print more
 * verbose information, including current file and line.
 *
 * MSC_TRACE()
 *
 *   Logs the current method/function if MSC_LOG_TRACE macro is defined and
 *   logLevel() is 'debug'.
 *
 * MSC_DEBUG(...)
 *
 * 	 Logs a debug.
 *
 * MSC_WARN(...)
 *
 *   Logs a warning.
 *
 * MSC_ERROR(...)
 *
 *   Logs an error.
 *
 * MSC_DUMP(...)
 *
 * 	 Logs always to stdout.
 *
 * MSC_ABORT(...)
 *
 *   Logs the given error to stderr and aborts the process.
 *
 * MSC_ASSERT(condition, ...)
 *
 *   If the condition is not satisfied, it calls MSC_ABORT().
 */

#ifndef MSC_LOGGER_HPP
#define MSC_LOGGER_HPP

#include <cstdio>  // std::snprintf(), std::fprintf(), stdout, stderr
#include <cstdlib> // std::abort()
#include <cstring>
#include "Utils.h"

namespace mediasoup {

class MS_EXPORT Logger
{
public:
	enum class LogLevel : int
	{
		LOG_NONE = 0,
		LOG_ERROR = 1,
		LOG_WARN = 2,
		LOG_DEBUG = 3,
		LOG_TRACE = 4
	};

	class LogHandlerInterface
	{
	public:
		virtual void OnLog(LogLevel level, const std::string& payload) = 0;
		virtual void OnLog(LogLevel level, const char* file, const char* function, const char* className, const std::string& payload) = 0;
	};

	class DefaultLogHandler : public LogHandlerInterface
	{
	public:
		DefaultLogHandler();
		virtual ~DefaultLogHandler();

		void OnLog(LogLevel level, const std::string& payload) override;
		void OnLog(LogLevel level, const char* file, const char* function, const char* className, const std::string& payload) override;
	};

	static void SetLogLevel(LogLevel level);
	static void SetHandler(LogHandlerInterface* handler);
	static void SetDefaultHandler();
	static void ShutDown();
public:
	static LogLevel logLevel();
	static LogHandlerInterface * handler();
};

}

// clang-format off

/* Logging macros. */



#define _MSC_LOG_SEPARATOR_CHAR "\n"

#ifdef MSC_LOG_FILE_LINE
#define _MSC_LOG_STR " %s:%d | %s::%s()"
#define _MSC_LOG_STR_DESC _MSC_LOG_STR " | "
#define _MSC_FILE (std::strchr(__FILE__, '/') ? std::strchr(__FILE__, '/') + 1 : __FILE__)
#define _MSC_LOG_ARG _MSC_FILE, __LINE__, MSC_CLASS, __FUNCTION__
#else
#define _MSC_LOG_STR " %s::%s()"
#define _MSC_LOG_STR_DESC _MSC_LOG_STR " | "
#define _MSC_LOG_ARG MSC_CLASS, __FUNCTION__
#endif

#ifdef MSC_LOG_TRACE
#define MSC_TRACE() \
		do \
		{ \
			if (Logger::handler() && Logger::logLevel() == Logger::LogLevel::LOG_DEBUG) \
			{ \
				std::string payload = Utils::Printf(desc, ##__VA_ARGS__); \
				Logger::handler()->OnLog(Logger::LogLevel::LOG_TRACE, payload); \
				Logger::handler()->OnLog(Logger::LogLevel::LOG_TRACE, __FILE__, __FUNCTION__, MSC_CLASS, payload); \
			} \
		} \
		while (false)
#else
#define MSC_TRACE() ;
#endif

#define MSC_DEBUG(desc, ...) \
	do \
	{ \
		if (Logger::handler() && Logger::logLevel() == Logger::LogLevel::LOG_DEBUG) \
		{ \
			std::string payload = Utils::Printf(desc, ##__VA_ARGS__); \
			Logger::handler()->OnLog(Logger::LogLevel::LOG_DEBUG, payload); \
			Logger::handler()->OnLog(Logger::LogLevel::LOG_DEBUG, __FILE__, __FUNCTION__, MSC_CLASS, payload); \
		} \
	} \
	while (false)

#define MSC_WARN(desc, ...) \
	do \
	{ \
		if (Logger::handler() && Logger::logLevel() >= Logger::LogLevel::LOG_WARN) \
		{ \
			std::string payload = Utils::Printf(desc, ##__VA_ARGS__); \
			Logger::handler()->OnLog(Logger::LogLevel::LOG_WARN, payload); \
			Logger::handler()->OnLog(Logger::LogLevel::LOG_WARN, __FILE__, __FUNCTION__, MSC_CLASS, payload); \
		} \
	} \
	while (false)

#define MSC_ERROR(desc, ...) \
	do \
	{ \
		if (Logger::handler() && Logger::logLevel() >= Logger::LogLevel::LOG_ERROR) \
		{ \
			std::string payload = Utils::Printf(desc, ##__VA_ARGS__); \
			Logger::handler()->OnLog(Logger::LogLevel::LOG_ERROR, payload); \
			Logger::handler()->OnLog(Logger::LogLevel::LOG_ERROR, __FILE__, __FUNCTION__, MSC_CLASS, payload); \
		} \
	} \
	while (false)

#define MSC_DUMP(desc, ...) \
	do \
	{ \
		std::fprintf(stdout, _MSC_LOG_STR_DESC desc _MSC_LOG_SEPARATOR_CHAR, _MSC_LOG_ARG, ##__VA_ARGS__); \
		std::fflush(stdout); \
	} \
	while (false)

#define MSC_ABORT(desc, ...) \
	do \
	{ \
		std::fprintf(stderr, "[ABORT]" _MSC_LOG_STR_DESC desc _MSC_LOG_SEPARATOR_CHAR, _MSC_LOG_ARG, ##__VA_ARGS__); \
		std::fflush(stderr); \
		std::abort(); \
	} \
	while (false)

#define MSC_ASSERT(condition, desc, ...) \
	if (!(condition)) \
	{ \
		MSC_ABORT("failed assertion `%s': " desc, #condition, ##__VA_ARGS__); \
	}

// clang-format on

#endif


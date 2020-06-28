#define MSC_CLASS "Logger"

#include "Logger.hpp"
#include "utils.hpp"
#include <iostream>

namespace mediasoupclient
{
	/* Class variables. */

	Logger::LogHandlerInterface* Logger::handler{ nullptr };
	char Logger::buffer[Logger::bufferSize];
	Logger::LogLevel Logger::logLevel = Logger::LogLevel::LOG_NONE;

	/* Class methods. */

	void Logger::SetLogLevel(Logger::LogLevel level)
	{
		Logger::logLevel = level;
	}

	void Logger::SetHandler(LogHandlerInterface* handler)
	{
		Logger::handler = handler;
	}

	void Logger::SetDefaultHandler()
	{
		Logger::handler = new Logger::DefaultLogHandler();
	}

	/* DefaultLogHandler */

	void Logger::DefaultLogHandler::OnLog(LogLevel /*level*/, char* payload, size_t /*len*/)
	{
		std::cout << payload << std::endl;
	}

	Logger::Logger(std::string prefix) 
		: _prefix(prefix)
	{

	}

	void Logger::debug(const char * a_Format, ...)
	{
		std::string res;
		va_list args;
		va_start(args, a_Format);
		utils::AppendVPrintf(res, a_Format, args);
		va_end(args);

		res = "[" + _prefix + "] " + res;

		Logger::handler->OnLog(Logger::LogLevel::LOG_DEBUG, res.data(), res.size());
	}

	void Logger::warn(const char * a_Format, ...)
	{
		std::string res;
		va_list args;
		va_start(args, a_Format);
		utils::AppendVPrintf(res, a_Format, args);
		va_end(args);

		res = "[" + _prefix + "] " + res;

		Logger::handler->OnLog(Logger::LogLevel::LOG_WARN, res.data(), res.size());
	}

	void Logger::error(const char * a_Format, ...)
	{
		std::string res;
		va_list args;
		va_start(args, a_Format);
		utils::AppendVPrintf(res, a_Format, args);
		va_end(args);

		res = "[" + _prefix + "] " + res;

		Logger::handler->OnLog(Logger::LogLevel::LOG_ERROR, res.data(), res.size());
	}

} // namespace mediasoupclient

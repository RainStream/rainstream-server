#define MS_CLASS "Logger"
// #define MS_LOG_DEV

#include "Logger.hpp"

/* Class variables. */

std::string Logger::id{ "unset" };
char Logger::buffer[Logger::bufferSize];

/* Class methods. */

void Logger::Init(const std::string& id)
{
	Logger::id = id;

	MS_TRACE();
}

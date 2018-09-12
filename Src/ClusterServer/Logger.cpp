#define MS_CLASS "Logger"
// #define MS_LOG_DEV

#include "Logger.hpp"

/* Class variables. */

std::string Logger::id{ "unset" };
//RTC::Channel* Logger::channel{ nullptr };
char Logger::buffer[Logger::bufferSize];

/* Class methods. */

// void Logger::Init(const std::string& id, RTC::Channel* channel)
// {
// 	Logger::id      = id;
// 	Logger::channel = channel;
// 
// 	MS_TRACE();
// }

void Logger::Init(const std::string& id)
{
	Logger::id = id;

	MS_TRACE();
}


#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#ifdef  _WIN32
#include <Windows.h>
#endif
// clang-format on

void cprintf(uint32_t color, char* format, ...)
{
	char buffer[4096] = { 0 };

		va_list args;
		va_start(args, format);
		vsprintf(buffer, format,  args);
		va_end(args);
#ifdef  _WIN32
	WORD colorOld;
	HANDLE handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(handle, &csbi);
	colorOld = csbi.wAttributes;
	SetConsoleTextAttribute(handle, color);
#endif
	std::cout << buffer;
	#ifdef  _WIN32
	SetConsoleTextAttribute(handle, colorOld);
#endif

}

#define MSC_CLASS "Logger"

#include "common.h"
#include "Logger.h"
#include <iostream>
#include <uv.h>
#include <chrono>
#include "Utils.h"

/* Class variables. */

Logger::LogHandlerInterface* Logger::handler{ nullptr };
char Logger::buffer[Logger::bufferSize];
Logger::LogLevel Logger::logLevel = Logger::LogLevel::LOG_NONE;

static uv_tty_t tty;
std::chrono::time_point<std::chrono::steady_clock> last_time_point;
std::map<std::string, std::string> classColorMap;


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

void Logger::ShutDown()
{
	if (Logger::handler)
	{
		delete Logger::handler;
	}

	Logger::handler = nullptr;
}

/* DefaultLogHandler */

Logger::DefaultLogHandler::DefaultLogHandler()
{
	// 目前只对 tty 控制台处理
	if (uv_guess_handle(1) != UV_TTY) {
		fprintf(stderr, "uv_guess_handle(1) != UV_TTY!\n");
		exit(EXIT_FAILURE);
	}

	uv_tty_init(uv_default_loop(), &tty, 1, 0);
	uv_tty_set_mode(&tty, UV_TTY_MODE_NORMAL);

	last_time_point = std::chrono::steady_clock::now();
}

Logger::DefaultLogHandler::~DefaultLogHandler()
{
	uv_tty_reset_mode();
}

void Logger::DefaultLogHandler::OnLog(LogLevel /*level*/, char* payload, size_t /*len*/)
{
	
}

void Logger::DefaultLogHandler::OnLog(LogLevel level, const char* file, const char* function, const char* className, char* payload, size_t len)
{
	auto current_time_point = std::chrono::steady_clock::now();
	int duration_millsecond = std::chrono::duration<double, std::milli>(current_time_point - last_time_point).count();
	last_time_point = current_time_point;

	std::string time_diff = " +" + std::to_string(duration_millsecond) + "ms";

	uv_buf_t buf[9];
	unsigned buf_size = sizeof buf / sizeof * buf;


	static std::map<Logger::LogLevel, std::string> tagColorMap= { 
		{LogLevel::LOG_ERROR,"\033[31;1m[ERROR]\033[0m "},
		{LogLevel::LOG_WARN,"\033[33;1m[WARN]\033[0m "},
		{LogLevel::LOG_DEBUG,"\033[32;1m[DEBUG]\033[0m "},
		{LogLevel::LOG_TRACE,"\033[36;1m[TRACE]\033[0m "}
	};


	std::string class_name = Utils::Printf("%s", function);

	std::string class_color_start = "\033[31m";
	std::string class_color_end = "\033[0m";

	if (!classColorMap.contains(class_name))
	{
		static int index = 31;
		classColorMap[class_name] = Utils::Printf("\033[%d;1m", index++);
		if (index > 36)
		{
			index = 31;
		}
	}

	class_color_start = classColorMap[class_name];

	class_name += " ";

	std::string end_line = "\n";

	// 开始发送消息

	buf[0].base = tagColorMap[level].data();
	buf[0].len = tagColorMap[level].length();

	buf[1].base = class_color_start.data();
	buf[1].len = class_color_start.length();

	buf[2].base = class_name.data();
	buf[2].len = class_name.length();

	buf[3].base = class_color_end.data();
	buf[3].len = class_color_end.length();

	buf[4].base = payload;
	buf[4].len = len;

	buf[5].base = class_color_start.data();
	buf[5].len = class_color_start.length();

	buf[6].base = time_diff.data();
	buf[6].len = time_diff.length();

	buf[7].base = class_color_end.data();
	buf[7].len = class_color_end.length();

	buf[8].base = end_line.data();
	buf[8].len = end_line.length();

	uv_try_write((uv_stream_t*)&tty, buf, buf_size);
}

#define MSC_CLASS "main"

#include <common.h>
#include <Logger.h>
#include "DepLibUV.hpp"
#include "Loop.hpp"
//#include "Settings.hpp"
#include "Utils.h"
#include "ClusterServer.hpp"
#include <uv.h>
#include <cerrno>
#include <csignal>  // sigaction()
#include <cstdlib>  // std::_Exit(), std::genenv()
#include <iostream> // std::cout, std::cerr, std::endl
#include <map>
#include <string>
#include <unistd.h> // getpid(), usleep()


static void init();
static void ignoreSignals();
static void destroy();
static void exitSuccess();
static void exitWithError();
static void printHelp();


int main(int argc, char* argv[])
{
	// Initialize libuv stuff (we need it for the Channel).
	DepLibUV::ClassInit();

	// Initialize the Logger.
	Logger::SetLogLevel(Logger::LogLevel::LOG_DEBUG);
	Logger::SetDefaultHandler();

	printHelp();

	// Setup the configuration.
// 	try
// 	{
// 		Settings::SetConfiguration(argc, argv);
// 	}
// 	catch (const RainStreamError& error)
// 	{
// 		MSC_ERROR("configuration error: %s", error.what());
// 
// 		exitWithError();
// 	}
// 
// 	// Print the effective configuration.
// 	Settings::PrintConfiguration();

	//	MS_DEBUG_TAG(info, "starting mediasoup-worker [pid:%ld]", (long)getpid());

#if defined(MS_LITTLE_ENDIAN)
	DLOG(INFO) << "Little-Endian CPU detected";
#elif defined(MS_BIG_ENDIAN)
	DLOG(INFO) << "Big-Endian CPU detected";
#endif

#if defined(INTPTR_MAX) && defined(INT32_MAX) && (INTPTR_MAX == INT32_MAX)
	MSC_DEBUG("32 bits architecture detected");
#elif defined(INTPTR_MAX) && defined(INT64_MAX) && (INTPTR_MAX == INT64_MAX)
	MSC_DEBUG("64 bits architecture detected");
#else
	MSC_DEBUG("can not determine whether the architecture is 32 or 64 bits");
#endif

	try
	{
		init();

		// Set the Server socket (this will be handled and deleted by the Loop).
		auto* server = new ClusterServer();

		// Run the Loop.
		Loop loop;

		delete server;

		// Loop ended.
		destroy();
		exitSuccess();
	}
	catch (const std::exception& error)
	{
		MSC_DEBUG("failure exit: %s", error.what());

		destroy();
		exitWithError();
	}
}


void init()
{
	ignoreSignals();
	DepLibUV::PrintVersion();

	// Initialize static stuff.
}

void ignoreSignals()
{
	// 	MSC_TRACE();
	// 
	// 	int err;
	// 	// clang-format off
	// 	struct sigaction act{};
	// 	std::map<std::string, int> ignoredSignals =
	// 	{
	// 		{ "PIPE", SIGPIPE },
	// 		{ "HUP",  SIGHUP  },
	// 		{ "ALRM", SIGALRM },
	// 		{ "USR1", SIGUSR2 },
	// 		{ "USR2", SIGUSR1}
	// 	};
	// 	// clang-format on
	// 
	// 	// Ignore clang-tidy cppcoreguidelines-pro-type-cstyle-cast.
	// 	act.sa_handler = SIG_IGN; // NOLINT
	// 	act.sa_flags   = 0;
	// 	err            = sigfillset(&act.sa_mask);
	// 	if (err != 0)
	// 		MSC_THROW_ERROR("sigfillset() failed: %s", std::strerror(errno));
	// 
	// 	for (auto& ignoredSignal : ignoredSignals)
	// 	{
	// 		auto& sigName = ignoredSignal.first;
	// 		int sigId     = ignoredSignal.second;
	// 
	// 		err = sigaction(sigId, &act, nullptr);
	// 		if (err != 0)
	// 		{
	// 			MSC_THROW_ERROR("sigaction() failed for signal %s: %s", sigName.c_str(), std::strerror(errno));
	// 		}
	// 	}
}

void destroy()
{
	// Free static stuff.
	DepLibUV::ClassDestroy();
}

void exitSuccess()
{
	// Wait a bit so peding messages to stdout/Channel arrive to the main process.
	usleep(100000);
	// And exit with success status.
	std::_Exit(EXIT_SUCCESS);
}

void exitWithError()
{
	// Wait a bit so peding messages to stderr arrive to the main process.
	usleep(100000);
	// And exit with error status.
	std::_Exit(EXIT_FAILURE);
}

void printHelp()
{
	const char* info =
		"\033[32;1m                        Cluster WebRTC SFU（MediaSoup）\n" \
		"--------------------------------------------------------------------------------\n" \
		"\033[36;1m声明\033[32;1m\n" \
		"--------------------------------------------------------------------------------\n" \
		"1、本系统基于mediasoup改造，使用c++语言替换nodesjs部分，完整保留原有协程架构；\n" \
		"2、SFU本身跨平台，编译Windows版本仅为测试方便；\n" \
		"3、客户端目前支持Web端，Windows、MacOS、Linux版本会相继推出；\n" \
		"4、移动端Android、IOS等平台可从github上找到对应版本；\n" \
		"5、仅提供一个进程实例，修改配置文件中的numWorkers无效；\n" \
		"6、本系统上属于开发测试阶段，存在bug在所难免，切勿用于生产环境，否则后果自负;\n" \
		"7、技术问题可加QQ群：\033[33;1m558729591\033[32;1m，商务合作可联系作者本人。\n" \
		"--------------------------------------------------------------------------------\n" \
		"\033[36;1m作者\033[32;1m\n" \
		"--------------------------------------------------------------------------------\n" \
		"blog: \033[33;1mhttps://blog.csdn.net/gupar\033[32;1m\n" \
		"github: \033[33;1mhttps://github.com/harvestsure\033[32;1m\n" \
		"email: \033[33;1mharvestsure@gmail.com\033[32;1m\n" \
		"QQ\\VX: \033[33;1m345252622\033[32;1m\n" \
		"Address: \033[33;1mBeijing, China\033[32;1m\n" \
		"DateTime: \033[33;1m2023-3-15\033[32;1m\n\n\033[0m";


	printf("%s", info);
	printf("%s", "按任意键开始执行......");
	getchar();
}


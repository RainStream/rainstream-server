﻿#define MS_CLASS "main"

#include "common.hpp"
#include "DepLibUV.hpp"
#include "Loop.hpp"
#include "RainStreamError.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
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


int main(int argc, char* argv[])
{
	// Initialize libuv stuff (we need it for the Channel).
	DepLibUV::ClassInit();

	// Initialize the Logger.
	google::InitGoogleLogging(argv[0]);    // 初始化
	google::SetLogDestination(google::GLOG_INFO, "log/prefix_");
	google::SetStderrLogging(google::GLOG_INFO);

	// Setup the configuration.
	try
	{
		Settings::SetConfiguration(argc, argv);
	}
	catch (const RainStreamError& error)
	{
		LOG(ERROR) << "configuration error: %s", error.what();

		exitWithError();
	}

	// Print the effective configuration.
	Settings::PrintConfiguration();

	//	MS_DEBUG_TAG(info, "starting mediasoup-worker [pid:%ld]", (long)getpid());

#if defined(MS_LITTLE_ENDIAN)
	LOG(INFO) << "Little-Endian CPU detected";
#elif defined(MS_BIG_ENDIAN)
	LOG(INFO) << "Big-Endian CPU detected";
#endif

#if defined(INTPTR_MAX) && defined(INT32_MAX) && (INTPTR_MAX == INT32_MAX)
	MS_DEBUG_TAG(info, "32 bits architecture detected");
#elif defined(INTPTR_MAX) && defined(INT64_MAX) && (INTPTR_MAX == INT64_MAX)
	LOG(INFO) << "64 bits architecture detected";
#else
	LOG(INFO) << "can not determine whether the architecture is 32 or 64 bits";
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
	catch (const RainStreamError& error)
	{
		LOG(INFO) << "failure exit: %s", error.what();

		destroy();
		exitWithError();
	}

	google::ShutdownGoogleLogging();
}


void init()
{
	ignoreSignals();
	DepLibUV::PrintVersion();

	// Initialize static stuff.
}

void ignoreSignals()
{
	// 	MS_TRACE();
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
	// 		RS_THROW_ERROR("sigfillset() failed: %s", std::strerror(errno));
	// 
	// 	for (auto& ignoredSignal : ignoredSignals)
	// 	{
	// 		auto& sigName = ignoredSignal.first;
	// 		int sigId     = ignoredSignal.second;
	// 
	// 		err = sigaction(sigId, &act, nullptr);
	// 		if (err != 0)
	// 		{
	// 			RS_THROW_ERROR("sigaction() failed for signal %s: %s", sigName.c_str(), std::strerror(errno));
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

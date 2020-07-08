﻿#define MSC_CLASS "main"

#include <common.hpp>
#include <Logger.hpp>
#include "DepLibUV.hpp"
#include "Loop.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "MediaServer.hpp"
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
	Logger::SetLogLevel(Logger::LogLevel::LOG_DEBUG);
	Logger::SetDefaultHandler();

	// Setup the configuration.
 	try
 	{
 		Settings::SetConfiguration(argc, argv);
 	}
 	catch (const std::exception& error)
 	{
 		MSC_ERROR("configuration error: %s", error.what());
 
 		exitWithError();
 	}
 
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

	// Print the effective configuration.
	Settings::PrintConfiguration();
	DepLibUV::PrintVersion();

	try
	{
		// Ignore some signals.
		ignoreSignals();

		// Set the Server socket (this will be handled and deleted by the Loop).
		auto* server = new MediaServer();

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

void ignoreSignals()
{
#ifndef _WIN32
	MSC_TRACE();

	int err;
	struct sigaction act; // NOLINT(cppcoreguidelines-pro-type-member-init)

	// clang-format off
	std::map<std::string, int> ignoredSignals =
	{
		{ "PIPE", SIGPIPE },
		{ "HUP",  SIGHUP  },
		{ "ALRM", SIGALRM },
		{ "USR1", SIGUSR1 },
		{ "USR2", SIGUSR2 }
	};
	// clang-format on

	act.sa_handler = SIG_IGN; // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	act.sa_flags = 0;
	err = sigfillset(&act.sa_mask);

	if (err != 0)
		MSC_THROW_ERROR("sigfillset() failed: %s", std::strerror(errno));

	for (auto& kv : ignoredSignals)
	{
		auto& sigName = kv.first;
		int sigId = kv.second;

		err = sigaction(sigId, &act, nullptr);

		if (err != 0)
			MSC_THROW_ERROR("sigaction() failed for signal %s: %s", sigName.c_str(), std::strerror(errno));
	}
#endif
}

void destroy()
{
	// Free static stuff.
	DepLibUV::ClassDestroy();
}

void exitSuccess()
{
	// Wait a bit so peding messages to stdout/Channel arrive to the main process.
	usleep(200);
	// And exit with success status.
	std::_Exit(EXIT_SUCCESS);
}

void exitWithError()
{
	// Wait a bit so peding messages to stderr arrive to the main process.
	usleep(200);
	// And exit with error status.
	std::_Exit(EXIT_FAILURE);
}

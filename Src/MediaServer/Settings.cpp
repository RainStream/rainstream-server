#define MSC_CLASS "Settings"
// #define MS_LOG_DEV

#include "Settings.hpp"
#include "Logger.hpp"
#include "errors.hpp"
#include "Utils.hpp"
#include <uv.h>
#include <cctype> // isprint()
#include <cerrno>
#include <iterator> // std::ostream_iterator
#include <sstream>  // std::ostringstream
#include <unistd.h> // close()
extern "C" {
#include <getopt.h>
}

/* Class variables. */

struct Settings::Configuration Settings::configuration;
// clang-format off

// clang-format on

/* Class methods. */

void Settings::SetConfiguration(int argc, char* argv[])
{
	MSC_TRACE();

	/* Variables for getopt. */

	int c;
	int optionIdx{ 0 };
	std::string stringValue;
	// clang-format off
	struct option options[] =
	{
		{ "serverUrl",            required_argument, nullptr, 'S' },
		{ "nodeId",               required_argument, nullptr, 'N' },
		{ "configFile",          optional_argument, nullptr, 'C' },
		{ nullptr, 0, nullptr, 0 }
	};
	// clang-format on

	/* Parse command line options. */

	opterr = 0; // Don't allow getopt to print error messages.
	while ((c = getopt_long_only(argc, argv, "", options, &optionIdx)) != -1)
	{
		if (optarg == nullptr)
			MSC_THROW_ERROR("unknown configuration parameter: %s", optarg);

		switch (c)
		{
			case 'S':
				stringValue = std::string(optarg);
				SetServerUrl(stringValue);
				break;
			case 'N':
				stringValue = std::string(optarg);
				SetNodeId(stringValue);
				break;

			case 'C':
				stringValue =                        std::string(optarg);
				Settings::configuration.configFile = stringValue;
				break;

			// Invalid option.
			case '?':
				if (isprint(optopt) != 0)
					MSC_THROW_ERROR("invalid option '-%c'", (char)optopt);
				else
					MSC_THROW_ERROR("unknown long option given as argument");

			// Valid option, but it requires and argument that is not given.
			case ':':
				MSC_THROW_ERROR("option '%c' requires an argument", (char)optopt);

			// This should never happen.
			default:
				MSC_THROW_ERROR("'default' should never happen");
		}
	}

	/* Post configuration. */

	// RTC must have at least 'IPv4' or 'IPv6'.
	if (Settings::configuration.serverUrl.empty())
		MSC_THROW_ERROR("at least serverUrl must be not empty");

	if (Settings::configuration.nodeId.empty())
		MSC_THROW_ERROR("at least nodeId must be not empty");
}

void Settings::PrintConfiguration()
{
	MSC_TRACE();

	MSC_DEBUG("<configuration>");

	if (!Settings::configuration.serverUrl.empty())
	{
		MSC_DEBUG("  serverUrl             : \"%s\"", Settings::configuration.serverUrl.c_str());
	}
	else
	{
		MSC_DEBUG("  serverUrl             : (unavailable)");
	}

	if (!Settings::configuration.nodeId.empty())
	{
		MSC_DEBUG("  nodeId                : \"%s\"", Settings::configuration.nodeId.c_str());
	}
	else
	{
		MSC_DEBUG("  nodeId                : (unavailable)");
	}

	if (!Settings::configuration.configFile.empty())
	{
		MSC_DEBUG("  configFile           : \"%s\"", Settings::configuration.configFile.c_str());
	}

	MSC_DEBUG("</configuration>");
}

void Settings::SetServerUrl(const std::string& ip)
{
	MSC_TRACE();

	if (ip == "true")
		return;

	if (ip.empty() || ip == "false")
	{
		Settings::configuration.serverUrl.clear();

		return;
	}

	Settings::configuration.serverUrl = ip;
}

void Settings::SetNodeId(const std::string& nodeId)
{
	MSC_TRACE();

	if (nodeId == "true")
		return;

	if (nodeId.empty() || nodeId == "false")
	{
		Settings::configuration.nodeId.clear();

		return;
	}

	Settings::configuration.nodeId = nodeId;
}


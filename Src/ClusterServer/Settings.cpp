#define MS_CLASS "Settings"
// #define MS_LOG_DEV

#include "Settings.hpp"
#include "Logger.hpp"
#include "RainStreamError.hpp"
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

/* Helpers declaration. */

static bool isBindableIp(const std::string& ip, int family, int* bindErrno);

/* Class variables. */

struct Settings::Configuration Settings::configuration;
// clang-format off
std::map<std::string, LogLevel> Settings::string2LogLevel =
{
	{ "debug", LogLevel::LOG_DEBUG },
	{ "warn",  LogLevel::LOG_WARN  },
	{ "error", LogLevel::LOG_ERROR }
};
std::map<LogLevel, std::string> Settings::logLevel2String =
{
	{ LogLevel::LOG_DEBUG, "debug" },
	{ LogLevel::LOG_WARN,  "warn"  },
	{ LogLevel::LOG_ERROR, "error" }
};
// clang-format on

/* Class methods. */

void Settings::SetConfiguration(int argc, char* argv[])
{
	MS_TRACE();

	/* Set default configuration. */

	SetDefaultRtcIP(AF_INET);
	SetDefaultRtcIP(AF_INET6);

	/* Variables for getopt. */

	int c;
	int optionIdx{ 0 };
	std::string stringValue;
	std::vector<std::string> logTags;
	// clang-format off
	struct option options[] =
	{
		{ "logLevel",            optional_argument, nullptr, 'l' },
		{ "logTag",              optional_argument, nullptr, 't' },
		{ "serverIP",            optional_argument, nullptr, 'I' },
		{ "serverPort",          optional_argument, nullptr, 'P' },
		{ "configFile",          optional_argument, nullptr, 'C' },
		{ nullptr, 0, nullptr, 0 }
	};
	// clang-format on

	/* Parse command line options. */

	opterr = 0; // Don't allow getopt to print error messages.
	while ((c = getopt_long_only(argc, argv, "", options, &optionIdx)) != -1)
	{
		if (optarg == nullptr)
			RS_THROW_ERROR("unknown configuration parameter: %s", optarg);

		switch (c)
		{
			case 'l':
				stringValue = std::string(optarg);
				SetLogLevel(stringValue);
				break;

			case 't':
				stringValue = std::string(optarg);
				logTags.push_back(stringValue);
				break;

			case 'I':
				stringValue = std::string(optarg);
				SetServerIP(stringValue);
				break;

			case 'P':
				Settings::configuration.serverPort = std::stoi(optarg);
				break;

			case 'C':
				stringValue =                        std::string(optarg);
				Settings::configuration.configFile = stringValue;
				break;

			// Invalid option.
			case '?':
				if (isprint(optopt) != 0)
					RS_THROW_ERROR("invalid option '-%c'", (char)optopt);
				else
					RS_THROW_ERROR("unknown long option given as argument");

			// Valid option, but it requires and argument that is not given.
			case ':':
				RS_THROW_ERROR("option '%c' requires an argument", (char)optopt);

			// This should never happen.
			default:
				RS_THROW_ERROR("'default' should never happen");
		}
	}

	/* Post configuration. */

	// Set logTags.
	if (!logTags.empty())
		Settings::SetLogTags(logTags);

	// RTC must have at least 'IPv4' or 'IPv6'.
	if (!Settings::configuration.hasIP)
		RS_THROW_ERROR("at least serverIP or rtcIPv6 must be enabled");
}

void Settings::PrintConfiguration()
{
	MS_TRACE();

	std::vector<std::string> logTags;
	std::ostringstream logTagsStream;

	if (Settings::configuration.logTags.info)
		logTags.emplace_back("info");
	if (Settings::configuration.logTags.ice)
		logTags.emplace_back("ice");
	if (Settings::configuration.logTags.dtls)
		logTags.emplace_back("dtls");
	if (Settings::configuration.logTags.rtp)
		logTags.emplace_back("rtp");
	if (Settings::configuration.logTags.srtp)
		logTags.emplace_back("srtp");
	if (Settings::configuration.logTags.rtcp)
		logTags.emplace_back("rtcp");
	if (Settings::configuration.logTags.rbe)
		logTags.emplace_back("rbe");
	if (Settings::configuration.logTags.rtx)
		logTags.emplace_back("rtx");

	if (!logTags.empty())
	{
		std::copy(
		    logTags.begin(), logTags.end() - 1, std::ostream_iterator<std::string>(logTagsStream, ","));
		logTagsStream << logTags.back();
	}

	MS_DEBUG_TAG(info, "<configuration>");

	MS_DEBUG_TAG(
	    info,
	    "  logLevel            : \"%s\"",
	    Settings::logLevel2String[Settings::configuration.logLevel].c_str());
	MS_DEBUG_TAG(info, "  logTags             : \"%s\"", logTagsStream.str().c_str());
	if (Settings::configuration.hasIP)
	{
		MS_DEBUG_TAG(info, "  serverIP             : \"%s\"", Settings::configuration.serverIP.c_str());
	}
	else
	{
		MS_DEBUG_TAG(info, "  serverIP             : (unavailable)");
	}

	MS_DEBUG_TAG(info, "  serverPort          : %" PRIu16, Settings::configuration.serverPort);

	if (!Settings::configuration.configFile.empty())
	{
		MS_DEBUG_TAG(
		    info, "  configFile           : \"%s\"", Settings::configuration.configFile.c_str());
	}

	MS_DEBUG_TAG(info, "</configuration>");
}

void Settings::SetDefaultRtcIP(int requestedFamily)
{
	MS_TRACE();

	int err;
	uv_interface_address_t* addresses;
	int numAddresses;
	std::string ipv4;
	std::string ipv6;
	int bindErrno;

	err = uv_interface_addresses(&addresses, &numAddresses);
	if (err != 0)
		MS_ABORT("uv_interface_addresses() failed: %s", uv_strerror(err));

	for (int i{ 0 }; i < numAddresses; ++i)
	{
		uv_interface_address_t address = addresses[i];

		// Ignore internal addresses.
		if (address.is_internal != 0)
			continue;

		int family;
		uint16_t port;
		std::string ip;

		Utils::IP::GetAddressInfo(
		    reinterpret_cast<struct sockaddr*>(&address.address.address4), &family, ip, &port);

		if (family != requestedFamily)
			continue;

		switch (family)
		{
			case AF_INET:
				// Ignore if already got an IPv4.
				if (!ipv4.empty())
					continue;

				// Check if it is bindable.
				if (!isBindableIp(ip, AF_INET, &bindErrno))
					continue;

				ipv4 = ip;
				break;

			case AF_INET6:
				// Ignore if already got an IPv6.
				if (!ipv6.empty())
					continue;

				// Check if it is bindable.
				if (!isBindableIp(ip, AF_INET6, &bindErrno))
					continue;

				ipv6 = ip;
				break;
		}
	}

	if (!ipv4.empty())
	{
		Settings::configuration.serverIP = ipv4;
		Settings::configuration.hasIP = true;
	}

// 	if (!ipv6.empty())
// 	{
// 		Settings::configuration.serverIP = ipv6;
// 		Settings::configuration.hasIP = true;
// 	}

	uv_free_interface_addresses(addresses, numAddresses);
}

void Settings::SetLogLevel(std::string& level)
{
	MS_TRACE();

	// Lowcase given level.
	level = Utils::String::ToLowerCase(level);

	if (Settings::string2LogLevel.find(level) == Settings::string2LogLevel.end())
		RS_THROW_ERROR("invalid value '%s' for logLevel", level.c_str());

	Settings::configuration.logLevel = Settings::string2LogLevel[level];
}

void Settings::SetServerIP(const std::string& ip)
{
	MS_TRACE();

	if (ip == "true")
		return;

	if (ip.empty() || ip == "false")
	{
		Settings::configuration.serverIP.clear();
		Settings::configuration.hasIP = false;

		return;
	}

	switch (Utils::IP::GetFamily(ip))
	{
		case AF_INET:
			if (ip == "0.0.0.0")
				RS_THROW_ERROR("serverIP cannot be '0.0.0.0'");
			Settings::configuration.serverIP = ip;
			Settings::configuration.hasIP = true;
			break;
		case AF_INET6:
			RS_THROW_ERROR("invalid IPv6 '%s' for serverIP", ip.c_str());
		default:
			RS_THROW_ERROR("invalid value '%s' for serverIP", ip.c_str());
	}

	int bindErrno;

	if (!isBindableIp(ip, AF_INET, &bindErrno))
		RS_THROW_ERROR("cannot bind on '%s' for serverIP: %s", ip.c_str(), std::strerror(bindErrno));
}


void Settings::SetLogTags(std::vector<std::string>& tags)
{
	MS_TRACE();

	// Reset logTags.
	struct LogTags newLogTags;

	Settings::configuration.logTags = newLogTags;

	for (auto& tag : tags)
	{
		if (tag == "info")
			Settings::configuration.logTags.info = true;
		else if (tag == "ice")
			Settings::configuration.logTags.ice = true;
		else if (tag == "dtls")
			Settings::configuration.logTags.dtls = true;
		else if (tag == "rtp")
			Settings::configuration.logTags.rtp = true;
		else if (tag == "srtp")
			Settings::configuration.logTags.srtp = true;
		else if (tag == "rtcp")
			Settings::configuration.logTags.rtcp = true;
		else if (tag == "rbe")
			Settings::configuration.logTags.rbe = true;
		else if (tag == "rtx")
			Settings::configuration.logTags.rtx = true;
	}
}


/* Helpers. */

bool isBindableIp(const std::string& ip, int family, int* bindErrno)
{
	MS_TRACE();

	// clang-format off
	struct sockaddr_storage bindAddr{};
	// clang-format on
	int bindSocket;
	int err{ 0 };
	bool success;

	switch (family)
	{
		case AF_INET:
			err = uv_ip4_addr(ip.c_str(), 0, reinterpret_cast<struct sockaddr_in*>(&bindAddr));
			if (err != 0)
				MS_ABORT("uv_ipv4_addr() failed: %s", uv_strerror(err));

			bindSocket = socket(AF_INET, SOCK_DGRAM, 0);
			if (bindSocket == -1)
				MS_ABORT("socket() failed: %s", std::strerror(errno));

			err = bind(
			    bindSocket, reinterpret_cast<const struct sockaddr*>(&bindAddr), sizeof(struct sockaddr_in));
			break;

		case AF_INET6:
			uv_ip6_addr(ip.c_str(), 0, reinterpret_cast<struct sockaddr_in6*>(&bindAddr));
			if (err != 0)
				MS_ABORT("uv_ipv6_addr() failed: %s", uv_strerror(err));
			bindSocket = socket(AF_INET6, SOCK_DGRAM, 0);
			if (bindSocket == -1)
				MS_ABORT("socket() failed: %s", std::strerror(errno));

			err = bind(
			    bindSocket,
			    reinterpret_cast<const struct sockaddr*>(&bindAddr),
			    sizeof(struct sockaddr_in6));
			break;

		default:
			MS_ABORT("unknown family");
	}

	if (err == 0)
	{
		success = true;
	}
	else
	{
		success    = false;
		*bindErrno = errno;
	}

	#ifdef _WIN32
	err = closesocket(bindSocket);
	#else
	err = close(bindSocket);
	#endif // _WIN32

	if (err != 0)
		MS_ABORT("close() failed: %s", std::strerror(errno));

	return success;
}

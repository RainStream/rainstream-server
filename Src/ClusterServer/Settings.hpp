#ifndef MS_SETTINGS_HPP
#define MS_SETTINGS_HPP

#include "common.hpp"
#include "LogLevel.hpp"
#include <map>
#include <string>
#include <vector>

class Settings
{
public:
	struct LogTags
	{
		bool info{ false };
		bool ice{ false };
		bool dtls{ false };
		bool rtp{ false };
		bool srtp{ false };
		bool rtcp{ false };
		bool rtx{ false };
		bool rbe{ false };
	};

public:
	// Struct holding the configuration.
	struct Configuration
	{
		LogLevel logLevel{ LogLevel::LOG_DEBUG };
		struct LogTags logTags;
		std::string serverIP;
		uint16_t serverPort{ 3443 };
		std::string configFile;
		// Private fields.
		bool hasIP{ false };
	};

public:
	static void SetConfiguration(int argc, char* argv[]);
	static void PrintConfiguration();

private:
	static void SetDefaultRtcIP(int requestedFamily);
	static void SetLogLevel(std::string& level);
	static void SetServerIP(const std::string& ip);
	static void SetRtcPorts();
	static void SetLogTags(std::vector<std::string>& tags);

public:
	static struct Configuration configuration;

private:
	static std::map<std::string, LogLevel> string2LogLevel;
	static std::map<LogLevel, std::string> logLevel2String;
};

#endif

#ifndef MS_SETTINGS_HPP
#define MS_SETTINGS_HPP

#include "common.hpp"
#include <map>
#include <string>
#include <vector>

class Settings
{
public:
	// Struct holding the configuration.
	struct Configuration
	{
		std::string serverUrl;
		std::string nodeId;
		std::string configFile;
	};

public:
	static void SetConfiguration(int argc, char* argv[]);
	static void PrintConfiguration();

private:
	static void SetServerUrl(const std::string& ip);
	static void SetNodeId(const std::string& nodeId);

public:
	static struct Configuration configuration;
};

#endif

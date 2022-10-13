#define MSC_CLASS "scalabilityMode"

#include "common.hpp"
#include "scalabilityModes.hpp"
#include "Logger.hpp"
#include <regex>


static const std::regex ScalabilityModeRegex(
	"^[LS]([1-9]\\d{0,1})T([1-9]\\d{0,1})(_KEY)?", std::regex_constants::ECMAScript);

// namespace mediasoupclient
// {
	json parseScalabilityMode(const std::string& scalabilityMode)
	{
		/* clang-format off */
		json jsonScalabilityMode
		{
			{ "spatialLayers",  1 },
			{ "temporalLayers", 1 },
			{ "ksvc", false}
		};
		/* clang-format on */

		std::smatch match;

		std::regex_match(scalabilityMode, match, ScalabilityModeRegex);

		if (!match.empty())
		{
			try
			{
				jsonScalabilityMode["spatialLayers"] = std::stoul(match[1].str());
				jsonScalabilityMode["temporalLayers"] = std::stoul(match[2].str());
				jsonScalabilityMode["ksvc"] = bool(match[3].matched);
			}
			catch (std::exception& e)
			{
				MSC_WARN("invalid scalabilityMode: %s", e.what());
			}
		}
		else
		{
			MSC_WARN("invalid scalabilityMode: %s", scalabilityMode.c_str());
		}

		return jsonScalabilityMode;
	}
//} // namespace mediasoupclient

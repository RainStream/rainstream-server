#pragma once 

const ScalabilityModeRegex =
	new RegExp("^[LS]([1-9]\\d{0,1})T([1-9]\\d{0,1})(_KEY)?");

struct ScalabilityMode
{
	uint32_t spatialLayers;
	uint32_t temporalLayers;
	bool ksvc;
};

ScalabilityMode parse(std::string scalabilityMode)
{
	const match = ScalabilityModeRegex.exec(scalabilityMode || "");

	if (match)
	{
		return {
			spatialLayers: Number(match[1]),
			temporalLayers : Number(match[2]),
			ksvc : Boolean(match[3])
		};
	}
	else
	{
		return {
			spatialLayers: 1,
			temporalLayers : 1,
			ksvc : false
		};
	}
}

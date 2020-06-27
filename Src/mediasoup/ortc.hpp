#pragma once 

#include "common.hpp"
//import * as h264 from "h264-profile-level-id";
#include "utils.hpp"
#include "errors.hpp"
#include "supportedRtpCapabilities.hpp"
#include "scalabilityModes.hpp"
#include "RtpParameters.hpp"
#include "SctpParameters.hpp"


namespace ortc
{
	void validateRtpCapabilities(json& caps);
	void validateRtpCodecCapability(json& codec);
	void validateRtcpFeedback(json& fb);
	void validateRtpHeaderExtension(json& ext);
	void validateRtpParameters(json& params);
	void validateRtpCodecParameters(json& codec);
	void validateRtpHeaderExtensionParameters(json& ext);
	void validateRtpEncodingParameters(json& encoding);
	void validateRtcpParameters(json& rtcp);
	void validateSctpCapabilities(json& caps);
	void validateNumSctpStreams(json& numStreams);
	void validateSctpParameters(json& params);
	void validateSctpStreamParameters(json& params);
	json generateRouterRtpCapabilities(json& mediaCodecs = json::array());
	json getProducerRtpParametersMapping(json& params, json& caps);
	json getConsumableRtpParameters(std::string kind, json& params, json& caps, json& rtpMapping);
	bool canConsume(json& consumableParams, json& caps);
	json getConsumerRtpParameters(json& consumableParams, json& caps);
	json getPipeConsumerRtpParameters(json& consumableParams, bool enableRtx = false);
}

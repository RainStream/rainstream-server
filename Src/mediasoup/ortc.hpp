#pragma once 

#include "common.hpp"

namespace ortc
{
	void validateRtpCapabilities(const json& caps);
	void validateRtpCodecCapability(const json& codec);
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
	bool canConsume(const json& consumableParams, const json& caps);
	json getConsumerRtpParameters(json& consumableParams, json& caps);
	json getPipeConsumerRtpParameters(const json& consumableParams, bool enableRtx = false);
}

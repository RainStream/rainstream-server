#define MSC_CLASS "ortc"

#include "common.hpp"
#include "ortc.hpp"
#include "Logger.hpp"
#include "errors.hpp"
#include "utils.hpp"
#include "supportedRtpCapabilities.hpp"
#include "scalabilityModes.hpp"
#include "SctpParameters.hpp"

#include "api/video_codecs/h264_profile_level_id.h"
#include "media/base/sdp_video_format_utils.h"

#include <algorithm> // std::find_if
#include <regex>
#include <stdexcept>
#include <string>


// Static functions declaration.
static bool isRtxCodec(const json& codec);
static bool matchCodecs(json& aCodec, json& bCodec, bool strict = false, bool modify = false);
static uint8_t getH264PacketizationMode(const json& codec);
static uint8_t getH264LevelAssimetryAllowed(const json& codec);
static std::string getH264ProfileLevelId(const json& codec);
static std::string getVP9ProfileId(const json& codec);

const std::list<int> DynamicPayloadTypes =
{
	100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
	111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
	122, 123, 124, 125, 126, 127, 96, 97, 98, 99
};

namespace ortc
{
	/**
	 * Validates RtpCapabilities. It may modify given data by adding missing
	 * fields with default values.
	 * It throws if invalid.
	 */
	void validateRtpCapabilities(json& caps)
	{
		MSC_TRACE();

		if (!caps.is_object())
			MSC_THROW_TYPE_ERROR("caps is not an object");

		auto codecsIt = caps.find("codecs");
		auto headerExtensionsIt = caps.find("headerExtensions");

		// codecs is optional. If unset, fill with an empty array.
		if (codecsIt != caps.end() && !codecsIt->is_array())
		{
			MSC_THROW_TYPE_ERROR("caps.codecs is not an array");
		}
		else if (codecsIt == caps.end())
		{
			caps["codecs"] = json::array();
			codecsIt = caps.find("codecs");
		}

		for (auto& codec : *codecsIt)
		{
			validateRtpCodecCapability(codec);
		}

		// headerExtensions is optional. If unset, fill with an empty array.
		if (headerExtensionsIt != caps.end() && !headerExtensionsIt->is_array())
		{
			MSC_THROW_TYPE_ERROR("caps.headerExtensions is not an array");
		}
		else if (headerExtensionsIt == caps.end())
		{
			caps["headerExtensions"] = json::array();
			headerExtensionsIt = caps.find("headerExtensions");
		}

		for (auto& ext : *headerExtensionsIt)
		{
			validateRtpHeaderExtension(ext);
		}
	}

	/**
	 * Validates RtpCodecCapability. It may modify given data by adding missing
	 * fields with default values.
	 * It throws if invalid.
	 */
	void validateRtpCodecCapability(json& codec)
	{
		MSC_TRACE();

		static const std::regex MimeTypeRegex(
			"^(audio|video)/(.+)", std::regex_constants::ECMAScript | std::regex_constants::icase);

		if (!codec.is_object())
			MSC_THROW_TYPE_ERROR("codec is not an object");

		auto mimeTypeIt = codec.find("mimeType");
		auto preferredPayloadTypeIt = codec.find("preferredPayloadType");
		auto clockRateIt = codec.find("clockRate");
		auto channelsIt = codec.find("channels");
		auto parametersIt = codec.find("parameters");
		auto rtcpFeedbackIt = codec.find("rtcpFeedback");

		// mimeType is mandatory.
		if (mimeTypeIt == codec.end() || !mimeTypeIt->is_string())
			MSC_THROW_TYPE_ERROR("missing codec.mimeType");

		std::smatch mimeTypeMatch;
		std::string regexTarget = mimeTypeIt->get<std::string>();
		std::regex_match(regexTarget, mimeTypeMatch, MimeTypeRegex);

		if (mimeTypeMatch.empty())
			MSC_THROW_TYPE_ERROR("invalid codec.mimeType");

		// Just override kind with media component of mimeType.
		codec["kind"] = mimeTypeMatch[1].str();

		// preferredPayloadType is optional.
		if (preferredPayloadTypeIt != codec.end() && !preferredPayloadTypeIt->is_number_integer())
			MSC_THROW_TYPE_ERROR("invalid codec.preferredPayloadType");

		// clockRate is mandatory.
		if (clockRateIt == codec.end() || !clockRateIt->is_number_integer())
			MSC_THROW_TYPE_ERROR("missing codec.clockRate");

		// channels is optional. If unset, set it to 1 (just if audio).
		if (codec["kind"] == "audio")
		{
			if (channelsIt == codec.end() || !channelsIt->is_number_integer())
				codec["channels"] = 1;
		}
		else
		{
			if (channelsIt != codec.end())
				codec.erase("channels");
		}

		// parameters is optional. If unset, set it to an empty object.
		if (parametersIt == codec.end() || !parametersIt->is_object())
		{
			codec["parameters"] = json::object();
			parametersIt = codec.find("parameters");
		}

		for (auto it = parametersIt->begin(); it != parametersIt->end(); ++it)
		{
			auto& key = it.key();
			auto& value = it.value();

			if (!value.is_string() && !value.is_number() && value != nullptr)
				MSC_THROW_TYPE_ERROR("invalid codec parameter");

			// Specific parameters validation.
			if (key == "apt")
			{
				if (!value.is_number_integer())
					MSC_THROW_TYPE_ERROR("invalid codec apt parameter");
			}
		}

		// rtcpFeedback is optional. If unset, set it to an empty array.
		if (rtcpFeedbackIt == codec.end() || !rtcpFeedbackIt->is_array())
		{
			codec["rtcpFeedback"] = json::array();
			rtcpFeedbackIt = codec.find("rtcpFeedback");
		}

		for (auto& fb : *rtcpFeedbackIt)
		{
			validateRtcpFeedback(fb);
		}
	}

	/**
	 * Validates RtcpFeedback. It may modify given data by adding missing
	 * fields with default values.
	 * It throws if invalid.
	 */
	void validateRtcpFeedback(json& fb)
	{
		MSC_TRACE();

		if (!fb.is_object())
			MSC_THROW_TYPE_ERROR("fb is not an object");

		auto typeIt = fb.find("type");
		auto parameterIt = fb.find("parameter");

		// type is mandatory.
		if (typeIt == fb.end() || !typeIt->is_string())
			MSC_THROW_TYPE_ERROR("missing fb.type");

		// parameter is optional. If unset set it to an empty string.
		if (parameterIt == fb.end() || !parameterIt->is_string())
			fb["parameter"] = "";
	}

	/**
	 * Validates RtpHeaderExtension. It may modify given data by adding missing
	 * fields with default values.
	 * It throws if invalid.
	 */
	void validateRtpHeaderExtension(json& ext)
	{
		MSC_TRACE();

		if (!ext.is_object())
			MSC_THROW_TYPE_ERROR("ext is not an object");

		auto kindIt = ext.find("kind");
		auto uriIt = ext.find("uri");
		auto preferredIdIt = ext.find("preferredId");
		auto preferredEncryptIt = ext.find("preferredEncrypt");
		auto directionIt = ext.find("direction");

		// kind is optional. If unset set it to an empty string.
		if (kindIt == ext.end() || !kindIt->is_string())
			ext["kind"] = "";

		kindIt = ext.find("kind");
		std::string kind = kindIt->get<std::string>();

		if (kind != "" && kind != "audio" && kind != "video")
			MSC_THROW_TYPE_ERROR("invalid ext.kind");

		// uri is mandatory.
		if (uriIt == ext.end() || !uriIt->is_string() || uriIt->get<std::string>().empty())
			MSC_THROW_TYPE_ERROR("missing ext.uri");

		// preferredId is mandatory.
		if (preferredIdIt == ext.end() || !preferredIdIt->is_number_integer())
			MSC_THROW_TYPE_ERROR("missing ext.preferredId");

		// preferredEncrypt is optional. If unset set it to false.
		if (preferredEncryptIt != ext.end() && !preferredEncryptIt->is_boolean())
			MSC_THROW_TYPE_ERROR("invalid ext.preferredEncrypt");
		else if (preferredEncryptIt == ext.end())
			ext["preferredEncrypt"] = false;

		// direction is optional. If unset set it to sendrecv.
		if (directionIt != ext.end() && !directionIt->is_string())
			MSC_THROW_TYPE_ERROR("invalid ext.direction");
		else if (directionIt == ext.end())
			ext["direction"] = "sendrecv";
	}

	/**
	 * Validates RtpParameters. It may modify given data by adding missing
	 * fields with default values.
	 * It throws if invalid.
	 */
	void validateRtpParameters(json& params)
	{
		MSC_TRACE();

		if (!params.is_object())
			MSC_THROW_TYPE_ERROR("params is not an object");

		auto midIt = params.find("mid");
		auto codecsIt = params.find("codecs");
		auto headerExtensionsIt = params.find("headerExtensions");
		auto encodingsIt = params.find("encodings");
		auto rtcpIt = params.find("rtcp");

		// mid is optional.
		if (midIt != params.end() && (!midIt->is_string() || midIt->get<std::string>().empty()))
		{
			MSC_THROW_TYPE_ERROR("params.mid is not a string");
		}

		// codecs is mandatory.
		if (codecsIt == params.end() || !codecsIt->is_array())
			MSC_THROW_TYPE_ERROR("missing params.codecs");

		for (auto& codec : *codecsIt)
		{
			validateRtpCodecParameters(codec);
		}

		// headerExtensions is optional. If unset, fill with an empty array.
		if (headerExtensionsIt != params.end() && !headerExtensionsIt->is_array())
		{
			MSC_THROW_TYPE_ERROR("params.headerExtensions is not an array");
		}
		else if (headerExtensionsIt == params.end())
		{
			params["headerExtensions"] = json::array();
			headerExtensionsIt = params.find("headerExtensions");
		}

		for (auto& ext : *headerExtensionsIt)
		{
			validateRtpHeaderExtensionParameters(ext);
		}

		// encodings is optional. If unset, fill with an empty array.
		if (encodingsIt != params.end() && !encodingsIt->is_array())
		{
			MSC_THROW_TYPE_ERROR("params.encodings is not an array");
		}
		else if (encodingsIt == params.end())
		{
			params["encodings"] = json::array();
			encodingsIt = params.find("encodings");
		}

		for (auto& encoding : *encodingsIt)
		{
			validateRtpEncodingParameters(encoding);
		}

		// rtcp is optional. If unset, fill with an empty object.
		if (rtcpIt != params.end() && !rtcpIt->is_object())
		{
			MSC_THROW_TYPE_ERROR("params.rtcp is not an object");
		}
		else if (rtcpIt == params.end())
		{
			params["rtcp"] = json::object();
			rtcpIt = params.find("rtcp");
		}

		validateRtcpParameters(*rtcpIt);
	}

	/**
	 * Validates RtpCodecParameters. It may modify given data by adding missing
	 * fields with default values.
	 * It throws if invalid.
	 */
	void validateRtpCodecParameters(json& codec)
	{
		MSC_TRACE();

		static const std::regex MimeTypeRegex(
			"^(audio|video)/(.+)", std::regex_constants::ECMAScript | std::regex_constants::icase);

		if (!codec.is_object())
			MSC_THROW_TYPE_ERROR("codec is not an object");

		auto mimeTypeIt = codec.find("mimeType");
		auto payloadTypeIt = codec.find("payloadType");
		auto clockRateIt = codec.find("clockRate");
		auto channelsIt = codec.find("channels");
		auto parametersIt = codec.find("parameters");
		auto rtcpFeedbackIt = codec.find("rtcpFeedback");

		// mimeType is mandatory.
		if (mimeTypeIt == codec.end() || !mimeTypeIt->is_string())
			MSC_THROW_TYPE_ERROR("missing codec.mimeType");

		std::smatch mimeTypeMatch;
		std::string regexTarget = mimeTypeIt->get<std::string>();
		std::regex_match(regexTarget, mimeTypeMatch, MimeTypeRegex);

		if (mimeTypeMatch.empty())
			MSC_THROW_TYPE_ERROR("invalid codec.mimeType");

		// payloadType is mandatory.
		if (payloadTypeIt == codec.end() || !payloadTypeIt->is_number_integer())
			MSC_THROW_TYPE_ERROR("missing codec.payloadType");

		// clockRate is mandatory.
		if (clockRateIt == codec.end() || !clockRateIt->is_number_integer())
			MSC_THROW_TYPE_ERROR("missing codec.clockRate");

		// Retrieve media kind from mimeType.
		auto kind = mimeTypeMatch[1].str();

		// channels is optional. If unset, set it to 1 (just for audio).
		if (kind == "audio")
		{
			if (channelsIt == codec.end() || !channelsIt->is_number_integer())
				codec["channels"] = 1;
		}
		else
		{
			if (channelsIt != codec.end())
				codec.erase("channels");
		}

		// parameters is optional. If unset, set it to an empty object.
		if (parametersIt == codec.end() || !parametersIt->is_object())
		{
			codec["parameters"] = json::object();
			parametersIt = codec.find("parameters");
		}

		for (auto it = parametersIt->begin(); it != parametersIt->end(); ++it)
		{
			const auto& key = it.key();
			auto& value = it.value();

			if (!value.is_string() && !value.is_number() && value != nullptr)
				MSC_THROW_TYPE_ERROR("invalid codec parameter");

			// Specific parameters validation.
			if (key == "apt")
			{
				if (!value.is_number_integer())
					MSC_THROW_TYPE_ERROR("invalid codec apt parameter");
			}
		}

		// rtcpFeedback is optional. If unset, set it to an empty array.
		if (rtcpFeedbackIt == codec.end() || !rtcpFeedbackIt->is_array())
		{
			codec["rtcpFeedback"] = json::array();
			rtcpFeedbackIt = codec.find("rtcpFeedback");
		}

		for (auto& fb : *rtcpFeedbackIt)
		{
			validateRtcpFeedback(fb);
		}
	}

	/**
	 * Validates RtpHeaderExtensionParameters. It may modify given data by adding missing
	 * fields with default values.
	 * It throws if invalid.
	 */
	void validateRtpHeaderExtensionParameters(json& ext)
	{
		MSC_TRACE();

		if (!ext.is_object())
			MSC_THROW_TYPE_ERROR("ext is not an object");

		auto uriIt = ext.find("uri");
		auto idIt = ext.find("id");
		auto encryptIt = ext.find("encrypt");
		auto parametersIt = ext.find("parameters");

		// uri is mandatory.
		if (uriIt == ext.end() || !uriIt->is_string() || uriIt->get<std::string>().empty())
		{
			MSC_THROW_TYPE_ERROR("missing ext.uri");
		}

		// id is mandatory.
		if (idIt == ext.end() || !idIt->is_number_integer())
			MSC_THROW_TYPE_ERROR("missing ext.id");

		// encrypt is optional. If unset set it to false.
		if (encryptIt != ext.end() && !encryptIt->is_boolean())
			MSC_THROW_TYPE_ERROR("invalid ext.encrypt");
		else if (encryptIt == ext.end())
			ext["encrypt"] = false;

		// parameters is optional. If unset, set it to an empty object.
		if (parametersIt == ext.end() || !parametersIt->is_object())
		{
			ext["parameters"] = json::object();
			parametersIt = ext.find("parameters");
		}

		for (auto it = parametersIt->begin(); it != parametersIt->end(); ++it)
		{
			auto& value = it.value();

			if (!value.is_string() && !value.is_number())
				MSC_THROW_TYPE_ERROR("invalid header extension parameter");
		}
	}

	/**
	 * Validates RtpEncodingParameters. It may modify given data by adding missing
	 * fields with default values.
	 * It throws if invalid.
	 */
	void validateRtpEncodingParameters(json& encoding)
	{
		MSC_TRACE();

		if (!encoding.is_object())
			MSC_THROW_TYPE_ERROR("encoding is not an object");

		auto ssrcIt = encoding.find("ssrc");
		auto ridIt = encoding.find("rid");
		auto rtxIt = encoding.find("rtx");
		auto dtxIt = encoding.find("dtx");
		auto scalabilityModeIt = encoding.find("scalabilityMode");

		// ssrc is optional.
		if (ssrcIt != encoding.end() && !ssrcIt->is_number_integer())
			MSC_THROW_TYPE_ERROR("invalid encoding.ssrc");

		// rid is optional.
		if (ridIt != encoding.end() && (!ridIt->is_string() || ridIt->get<std::string>().empty()))
		{
			MSC_THROW_TYPE_ERROR("invalid encoding.rid");
		}

		// rtx is optional.
		if (rtxIt != encoding.end() && !rtxIt->is_object())
		{
			MSC_THROW_TYPE_ERROR("invalid encoding.rtx");
		}
		else if (rtxIt != encoding.end())
		{
			auto rtxSsrcIt = rtxIt->find("ssrc");

			// RTX ssrc is mandatory if rtx is present.
			if (rtxSsrcIt == rtxIt->end() || !rtxSsrcIt->is_number_integer())
				MSC_THROW_TYPE_ERROR("missing encoding.rtx.ssrc");
		}

		// dtx is optional. If unset set it to false.
		if (dtxIt == encoding.end() || !dtxIt->is_boolean())
			encoding["dtx"] = false;

		// scalabilityMode is optional.
		// clang-format off
		if (
			scalabilityModeIt != encoding.end() &&
			(!scalabilityModeIt->is_string() || scalabilityModeIt->get<std::string>().empty())
			)
			// clang-format on
		{
			MSC_THROW_TYPE_ERROR("invalid encoding.scalabilityMode");
		}
	}

	/**
	 * Validates RtcpParameters. It may modify given data by adding missing
	 * fields with default values.
	 * It throws if invalid.
	 */
	void validateRtcpParameters(json& rtcp)
	{
		MSC_TRACE();

		if (!rtcp.is_object())
			MSC_THROW_TYPE_ERROR("rtcp is not an object");

		auto cnameIt = rtcp.find("cname");
		auto reducedSizeIt = rtcp.find("reducedSize");

		// cname is optional.
		if (cnameIt != rtcp.end() && !cnameIt->is_string())
			MSC_THROW_TYPE_ERROR("invalid rtcp.cname");

		// reducedSize is optional. If unset set it to true.
		if (reducedSizeIt == rtcp.end() || !reducedSizeIt->is_boolean())
			rtcp["reducedSize"] = true;
	}

	/**
	 * Validates SctpCapabilities. It may modify given data by adding missing
	 * fields with default values.
	 * It throws if invalid.
	 */
	void validateSctpCapabilities(json& caps)
	{
		MSC_TRACE();

		if (!caps.is_object())
			MSC_THROW_TYPE_ERROR("caps is not an object");

		auto numStreamsIt = caps.find("numStreams");

		// numStreams is mandatory.
		if (numStreamsIt == caps.end() || !numStreamsIt->is_object())
			MSC_THROW_TYPE_ERROR("missing caps.numStreams");

		validateNumSctpStreams(*numStreamsIt);
	}

	/**
	 * Validates NumSctpStreams. It may modify given data by adding missing
	 * fields with default values.
	 * It throws if invalid.
	 */
	void validateNumSctpStreams(json& numStreams)
	{
		MSC_TRACE();

		if (!numStreams.is_object())
			MSC_THROW_TYPE_ERROR("numStreams is not an object");

		auto osIt = numStreams.find("OS");
		auto misIt = numStreams.find("MIS");

		// OS is mandatory.
		if (osIt == numStreams.end() || !osIt->is_number_integer())
			MSC_THROW_TYPE_ERROR("missing numStreams.OS");

		// MIS is mandatory.
		if (misIt == numStreams.end() || !misIt->is_number_integer())
			MSC_THROW_TYPE_ERROR("missing numStreams.MIS");
	}

	/**
	 * Validates SctpParameters. It may modify given data by adding missing
	 * fields with default values.
	 * It throws if invalid.
	 */
	void validateSctpParameters(json& params)
	{
		MSC_TRACE();

		if (!params.is_object())
			MSC_THROW_TYPE_ERROR("params is not an object");

		auto portIt = params.find("port");
		auto osIt = params.find("OS");
		auto misIt = params.find("MIS");
		auto maxMessageSizeIt = params.find("maxMessageSize");

		// port is mandatory.
		if (portIt == params.end() || !portIt->is_number_integer())
			MSC_THROW_TYPE_ERROR("missing params.port");

		// OS is mandatory.
		if (osIt == params.end() || !osIt->is_number_integer())
			MSC_THROW_TYPE_ERROR("missing params.OS");

		// MIS is mandatory.
		if (misIt == params.end() || !misIt->is_number_integer())
			MSC_THROW_TYPE_ERROR("missing params.MIS");

		// maxMessageSize is mandatory.
		if (maxMessageSizeIt == params.end() || !maxMessageSizeIt->is_number_integer())
		{
			MSC_THROW_TYPE_ERROR("missing params.maxMessageSize");
		}
	}

	/**
	 * Validates SctpStreamParameters. It may modify given data by adding missing
	 * fields with default values.
	 * It throws if invalid.
	 */
	void validateSctpStreamParameters(json& params)
	{
		MSC_TRACE();

		if (!params.is_object())
			MSC_THROW_TYPE_ERROR("params is not an object");

		auto streamIdIt = params.find("streamId");
		auto orderedIt = params.find("ordered");
		auto maxPacketLifeTimeIt = params.find("maxPacketLifeTime");
		auto maxRetransmitsIt = params.find("maxRetransmits");
		auto labelIt = params.find("label");
		auto protocolIt = params.find("protocol");

		// streamId is mandatory.
		if (streamIdIt == params.end() || !streamIdIt->is_number_integer())
			MSC_THROW_TYPE_ERROR("missing params.streamId");

		// ordered is optional.
		bool orderedGiven = false;

		if (orderedIt != params.end() && orderedIt->is_boolean())
			orderedGiven = true;
		else
			params["ordered"] = true;

		// maxPacketLifeTime is optional.
		if (maxPacketLifeTimeIt != params.end() && !maxPacketLifeTimeIt->is_number_integer())
		{
			MSC_THROW_TYPE_ERROR("invalid params.maxPacketLifeTime");
		}

		// maxRetransmits is optional.
		if (maxRetransmitsIt != params.end() || !maxRetransmitsIt->is_number_integer())
		{
			MSC_THROW_TYPE_ERROR("invalid params.maxRetransmits");
		}

		if (maxPacketLifeTimeIt != params.end() && maxRetransmitsIt != params.end())
		{
			MSC_THROW_TYPE_ERROR("cannot provide both maxPacketLifeTime and maxRetransmits");
		}

		// clang-format off
		if (
			orderedGiven &&
			params["ordered"] == true &&
			(maxPacketLifeTimeIt != params.end() || maxRetransmitsIt != params.end())
			)
			// clang-format on
		{
			MSC_THROW_TYPE_ERROR("cannot be ordered with maxPacketLifeTime or maxRetransmits");
		}
		// clang-format off
		else if (
			!orderedGiven &&
			(maxPacketLifeTimeIt != params.end() || maxRetransmitsIt != params.end())
			)
			// clang-format on
		{
			params["ordered"] = false;
		}
	}

	/**
	 * Generate RTP capabilities for the Router based on the given media codecs and
	 * mediasoup supported RTP capabilities.
	 */
	json generateRouterRtpCapabilities(json& mediaCodecs/* = json::array()*/)
	{
		// Normalize supported RTP capabilities.
		validateRtpCapabilities(supportedRtpCapabilities);

		if (!mediaCodecs.is_array())
			MSC_THROW_TYPE_ERROR("mediaCodecs must be an Array");

		json clonedSupportedRtpCapabilities = Utils::clone(supportedRtpCapabilities);
		std::list<int> dynamicPayloadTypes = DynamicPayloadTypes;
		json caps =
		{
			{ "codecs", json::array() },
			{ "headerExtensions", clonedSupportedRtpCapabilities["headerExtensions"] }
		};

		for (auto& mediaCodec : mediaCodecs)
		{
			// This may throw.
			validateRtpCodecCapability(mediaCodec);

			json clonedSupportedCodec = clonedSupportedRtpCapabilities["codecs"];

			auto matchedSupportedCodecIt = std::find_if(
				clonedSupportedCodec.begin(), clonedSupportedCodec.end(), [&](json& supportedCodec) {
				return matchCodecs(mediaCodec, supportedCodec, false);
			});

			if (matchedSupportedCodecIt == clonedSupportedCodec.end())
			{
				MSC_THROW_UNSUPPORTED_ERROR(
					"media codec not supported[mimeType:%s]", 
					mediaCodec["mimeType"].get<std::string>().c_str());
			}

			// Clone the supported codec.
			json codec = Utils::clone(*matchedSupportedCodecIt);

			// If the given media codec has preferredPayloadType, keep it.
			if (mediaCodec["preferredPayloadType"].is_number_integer())
			{
				codec["preferredPayloadType"] = mediaCodec["preferredPayloadType"];

				// Also remove the pt from the list of available dynamic values.
				auto iter = std::find(dynamicPayloadTypes.begin(),
					dynamicPayloadTypes.end(), codec["preferredPayloadType"]);

				if (iter != dynamicPayloadTypes.end())
					dynamicPayloadTypes.assign(iter, iter++);
			}
			// Otherwise if the supported codec has preferredPayloadType, use it.
			else if (codec["preferredPayloadType"].is_number_integer())
			{
				// No need to remove it from the list since it's not a dynamic value.
			}
			// Otherwise choose a dynamic one.
			else
			{
				// Take the first available pt and remove it from the list.
				int pt = dynamicPayloadTypes.front();
				dynamicPayloadTypes.pop_front();

				if (!pt)
					MSC_THROW_ERROR("cannot allocate more dynamic codec payload types");

				codec["preferredPayloadType"] = pt;
			}

			// Ensure there is not duplicated preferredPayloadType values.
			json& capsCodecs = caps["codecs"];
			auto matchedCapsCodecsIt = std::find_if(
				capsCodecs.begin(), capsCodecs.end(), [=](const json& c) {
				return c["preferredPayloadType"] == codec["preferredPayloadType"].get<int>();
			});
			if (matchedCapsCodecsIt != capsCodecs.end())
				MSC_THROW_TYPE_ERROR("duplicated codec.preferredPayloadType");

			// Merge the media codec parameters.
			json jParameters = codec["parameters"];
			jParameters.merge_patch(mediaCodec["parameters"]);
			codec["parameters"] = jParameters;

			// Append to the codec list.
			caps["codecs"].push_back(codec);

			// Add a RTX video codec if video.
			if (codec["kind"] == "video")
			{
				// Take the first available pt and remove it from the list.
				int pt = dynamicPayloadTypes.front();
				dynamicPayloadTypes.pop_front();

				if (!pt)
					MSC_THROW_ERROR("cannot allocate more dynamic codec payload types");

				json rtxCodec =
				{
					{ "kind", codec["kind"] },
					{ "mimeType", codec["kind"].get<std::string>().append("/rtx") },
					{ "preferredPayloadType", pt },
					{ "clockRate", codec["clockRate"] },
					{ "parameters",
						{
							{ "apt", codec["preferredPayloadType"] }
						}
					},
					{ "rtcpFeedback", json::array() }
				};

				// Append to the codec list.
				caps["codecs"].push_back(rtxCodec);
			}
		}

		return caps;
	}

	/**
	 * Get a mapping of codec payloads and encodings of the given Producer RTP
	 * parameters as values expected by the Router.
	 *
	 * It may throw if invalid or non supported RTP parameters are given.
	 */
	json getProducerRtpParametersMapping(json& params, json& caps)
	{
		json rtpMapping =
		{
			{ "codecs", json::array() },
			{ "encodings", json::array() }
		};

		// Match parameters media codecs to capabilities media codecs.
		std::map<json, json> codecToCapCodec;

		for (auto& codec : params["codecs"])
		{
			if (isRtxCodec(codec))
				continue;

			// Search for the same media codec in capabilities.
			json& capsCodecs = caps["codecs"];
			auto matchedCapCodecIt = std::find_if(
				capsCodecs.begin(), capsCodecs.end(), [&](json& capCodec) {
				return matchCodecs(codec, capCodec, true, true);
			});

			if (matchedCapCodecIt == capsCodecs.end())
			{
				MSC_THROW_UNSUPPORTED_ERROR(
					"unsupported codec[mimeType:%s, payloadType :%d]", 
					codec["mimeType"].get<std::string>().c_str(),
					codec["payloadType"].get<int>());
			}

			codecToCapCodec.insert(std::make_pair(codec, *matchedCapCodecIt));
		}

		// Match parameters RTX codecs to capabilities RTX codecs.
		for (auto& codec : params["codecs"])
		{
			if (!isRtxCodec(codec))
				continue;

			// Search for the associated media codec.

			json& mediaCodecs = params["codecs"];
			auto associatedMediaCodecIt = std::find_if(
				mediaCodecs.begin(), mediaCodecs.end(), [&](const json& mediaCodec) {
				return mediaCodec["payloadType"] == codec["parameters"]["apt"];
			});

			if (associatedMediaCodecIt == mediaCodecs.end())
			{
				MSC_THROW_TYPE_ERROR(
					"missing media codec found for RTX PT %s", 
					codec["payloadType"].get<std::string>().c_str());
			}

			json capMediaCodec = codecToCapCodec.at(*associatedMediaCodecIt);

			// Ensure that the capabilities media codec has a RTX codec.
			json& capsCodecs = caps["codecs"];
			auto associatedCapRtxCodecIt = std::find_if(
				capsCodecs.begin(), capsCodecs.end(), [&](const json& capCodec) {
				return isRtxCodec(capCodec) &&
					capCodec["parameters"]["apt"] == capMediaCodec["preferredPayloadType"].get<int>();
			});

			if (associatedCapRtxCodecIt == capsCodecs.end())
			{
				MSC_THROW_UNSUPPORTED_ERROR(
					"no RTX codec for capability codec PT %d", 
					capMediaCodec["preferredPayloadType"].get<int>());
			}

			codecToCapCodec.insert(std::make_pair(codec, *associatedCapRtxCodecIt));
		}

		// Generate codecs mapping.
		for (auto &[codec, capCodec] : codecToCapCodec)
		{
			rtpMapping["codecs"].push_back(
				{
					{ "payloadType", codec["payloadType"] },
					{ "mappedPayloadType", capCodec["preferredPayloadType"] }
				});
		}

		// Generate encodings mapping.
		uint32_t mappedSsrc = Utils::generateRandomNumber();

		for (auto& encoding : params["encodings"])
		{
			json mappedEncoding = json::object();

			mappedEncoding["mappedSsrc"] = mappedSsrc++;

			if (encoding.count("rid"))
				mappedEncoding["rid"] = encoding["rid"];
			if (encoding.count("ssrc"))
				mappedEncoding["ssrc"] = encoding["ssrc"];
			if (encoding.count("scalabilityMode"))
				mappedEncoding["scalabilityMode"] = encoding["scalabilityMode"];

			rtpMapping["encodings"].push_back(mappedEncoding);
		}

		return rtpMapping;
	}

/**
 * Generate RTP parameters to be internally used by Consumers given the RTP
 * parameters of a Producer and the RTP capabilities of the Router.
 */
json getConsumableRtpParameters(std::string kind, json& params, json& caps, json& rtpMapping)
{
	json consumableParams =
	{
		{ "codecs", json::array() },
		{ "headerExtensions", json::array() },
		{ "encodings", json::array() },
		{ "rtcp", json::object() }
	};

	for (auto& codec : params["codecs"])
	{
		if (isRtxCodec(codec))
			continue;

		json& rtpMappingCodecs = rtpMapping["codecs"];
		auto consumableCodecPtIt = std::find_if(
			rtpMappingCodecs.begin(), rtpMappingCodecs.end(), [&](const json& entry) {
			return entry["payloadType"] == codec["payloadType"];
		});

		uint32_t consumableCodecPt = (*consumableCodecPtIt)["mappedPayloadType"];

		json& capsCodecs = caps["codecs"];
		auto matchedCapCodecIt = std::find_if(
			capsCodecs.begin(), capsCodecs.end(), [&](const json& capCodec) {
			return capCodec["preferredPayloadType"] == consumableCodecPt;
		});

		json& matchedCapCodec = *matchedCapCodecIt;

		json consumableCodec =
		{
			{ "mimeType", matchedCapCodec["mimeType"] },
			{ "payloadType", matchedCapCodec["preferredPayloadType"] },
			{ "clockRate", matchedCapCodec["clockRate"] },			
			{ "parameters", codec["parameters"] }, // Keep the Producer codec parameters.
			{ "rtcpFeedback", matchedCapCodec["rtcpFeedback"] }
		};

		if (matchedCapCodec.contains("channels") && matchedCapCodec.value("channels", 0))
		{
			consumableCodec["channels"] = matchedCapCodec["channels"];
		}

		consumableParams["codecs"].push_back(consumableCodec);

		auto consumableCapRtxCodecIt = std::find_if(
			capsCodecs.begin(), capsCodecs.end(), [&](const json& capRtxCodec) {
			return isRtxCodec(capRtxCodec) &&
				capRtxCodec["parameters"]["apt"] == consumableCodec["payloadType"];
		});

		if (consumableCapRtxCodecIt!= capsCodecs.end())
		{
			json& consumableCapRtxCodec = *consumableCapRtxCodecIt;
			json consumableRtxCodec =
			{
				{ "mimeType", consumableCapRtxCodec["mimeType"] },
				{ "payloadType", consumableCapRtxCodec["preferredPayloadType"] },
				{ "clockRate", consumableCapRtxCodec["clockRate"] },
				{ "parameters", consumableCapRtxCodec["parameters"] }, // Keep the Producer codec parameters.
				{ "rtcpFeedback", consumableCapRtxCodec["rtcpFeedback"] }
			};

			consumableParams["codecs"].push_back(consumableRtxCodec);
		}
	}

	for (auto& capExt : caps["headerExtensions"])
	{
		// Just take RTP header extension that can be used in Consumers.
		if (
			capExt["kind"] != kind ||
			(capExt["direction"] != "sendrecv" && capExt["direction"] != "sendonly")
			)
		{
			continue;
		}

		json consumableExt =
		{
			{ "uri", capExt["uri"] },
			{ "id", capExt["preferredId"] },
			{ "encrypt", capExt["preferredEncrypt"] },
			{ "parameters", {} }
		};

		consumableParams["headerExtensions"].push_back(consumableExt);
	}

	// Clone Producer encodings since we'll mangle them.
	json consumableEncodings = Utils::clone(params["encodings"]);

	for (int i = 0; i < consumableEncodings.size(); ++i)
	{
		json& consumableEncoding = consumableEncodings[i];
		uint32_t mappedSsrc = rtpMapping["encodings"][i]["mappedSsrc"];

		// Remove useless fields.
		consumableEncoding.erase("rid");
		consumableEncoding.erase("rtx");
		consumableEncoding.erase("codecPayloadType");

		// Set the mapped ssrc.
		consumableEncoding["ssrc"] = mappedSsrc;

		consumableParams["encodings"].push_back(consumableEncoding);
	}

	consumableParams["rtcp"] =
	{
		{ "cname", params["rtcp"]["cname"] },
		{ "reducedSize", true },
		{ "mux", true }
	};

	return consumableParams;
}

/**
 * Check whether the given RTP capabilities can consume the given Producer.
 */
bool canConsume(json& consumableParams, json& caps)
{
	// This may throw.
	validateRtpCapabilities(caps);

	json matchingCodecs = json::array();

	for (auto& codec : consumableParams["codecs"])
	{
		const  json& capCodecs = caps["codecs"];

		auto matchedCapCodecIt =
			std::find_if(capCodecs.begin(), capCodecs.end(), [&](json capCodec) {
			return matchCodecs(capCodec, codec, true);
		});

		if (matchedCapCodecIt == capCodecs.end())
			continue;

		matchingCodecs.push_back(codec);
	}

	// Ensure there is at least one media codec.
	if (matchingCodecs.size() == 0 || isRtxCodec(matchingCodecs[0]))
		return false;

	return true;
}

/**
 * Generate RTP parameters for a specific Consumer.
 *
 * It reduces encodings to just one and takes into account given RTP capabilities
 * to reduce codecs, codecs' RTCP feedback and header extensions, and also enables
 * or disabled RTX.
 */
json getConsumerRtpParameters(json& consumableParams, json& caps, bool pipe)
{
	json consumerParams =
	{
		{ "codecs", json::array() },
		{ "headerExtensions", json::array() },
		{ "encodings", json::array() },
		{ "rtcp", consumableParams["rtcp"] }
	};

	for (auto& capCodec : caps["codecs"])
	{
		validateRtpCodecCapability(capCodec);
	}

	json consumableCodecs = Utils::clone(consumableParams["codecs"]);

	bool rtxSupported = false;

	for (auto& codec : consumableCodecs)
	{
		json& capsCodecs = caps["codecs"];
		auto matchedCapCodecIt = std::find_if(
			capsCodecs.begin(), capsCodecs.end(), [&](json& capCodec) {
			return matchCodecs(capCodec, codec, true);
		});

		if (matchedCapCodecIt == capsCodecs.end())
			continue;

		codec["rtcpFeedback"] = (*matchedCapCodecIt)["rtcpFeedback"];

		consumerParams["codecs"].push_back(codec);
	}

	// Must sanitize the list of matched codecs by removing useless RTX codecs.
	for (int idx = consumerParams["codecs"].size() - 1;  idx >= 0; --idx)
	{
		json& codec = consumerParams["codecs"][idx];

		if (isRtxCodec(codec))
		{
			json& consumerParamsCodecs = consumerParams["codecs"];

			auto matchedCapCodecIt = std::find_if(
				consumerParamsCodecs.begin(), consumerParamsCodecs.end(), [&](json& mediaCodec) {
					return mediaCodec["payloadType"] == codec["parameters"]["apt"];
				});

			if (matchedCapCodecIt != consumerParamsCodecs.end())
			{
				rtxSupported = true;
			}
			else
			{
				consumerParams["codecs"].erase(idx);
			}
		}
	}

	// Ensure there is at least one media codec.
	if (consumerParams["codecs"].size() == 0 || isRtxCodec(consumerParams["codecs"][0]))
	{
		MSC_THROW_UNSUPPORTED_ERROR("no compatible media codecs");
	}

	for (auto& ext : consumableParams["headerExtensions"])
	{
		for (auto& capExt : caps["headerExtensions"])
		{
			if (capExt["preferredId"] == ext["id"] && capExt["uri"] == ext["uri"])
			{
				consumerParams["headerExtensions"].push_back(ext);
			}
		}
	}

	// Reduce codecs' RTCP feedback. Use Transport-CC if available, REMB otherwise.
	auto isContainsExtension = [=](std::string uri)->bool
	{
		const json& consumerHeaderExtensions = consumerParams["headerExtensions"];

		auto matchedExtensionIt = std::find_if(
			consumerHeaderExtensions.begin(), consumerHeaderExtensions.end(), [&](const json& ext) {
			return ext["uri"] == uri;
		});

		return matchedExtensionIt != consumerHeaderExtensions.end();
	};

	if (isContainsExtension("http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01"))
	{
		for (auto& codec : consumerParams["codecs"])
		{
			json rtcpFeedback = codec["rtcpFeedback"];
			codec["rtcpFeedback"].clear();
			for (auto& fb : rtcpFeedback)
			{
				if (fb["type"] != "goog-remb")
				{
					codec["rtcpFeedback"].push_back(fb);
				}
			}
		}
	}
	else if (isContainsExtension("http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time"))
	{
		for (auto& codec : consumerParams["codecs"])
		{
			json rtcpFeedback = codec["rtcpFeedback"];
			codec["rtcpFeedback"].clear();
			for (auto& fb : rtcpFeedback)
			{
				if (fb["type"] != "transport-cc")
				{
					codec["rtcpFeedback"].push_back(fb);
				}
			}
		}
	}
	else
	{
		for (auto& codec : consumerParams["codecs"])
		{
			json rtcpFeedback = codec["rtcpFeedback"];
			codec["rtcpFeedback"].clear();
			for (auto& fb : rtcpFeedback)
			{
				if (fb["type"] != "transport-cc" &&
					fb["type"] != "goog-remb")
				{
					codec["rtcpFeedback"].push_back(fb);
				}
			}
		}
	}

	if (!pipe)
	{
		uint32_t ssrc = Utils::generateRandomNumber();
		json consumerEncoding =
		{
			{ "ssrc", ssrc }
		};

		if (rtxSupported)
			consumerEncoding["rtx"] = { { "ssrc", ssrc + 1} };

		// If any of the consumableParams.encodings has scalabilityMode, process it
		// (assume all encodings have the same value).
		json& consumableParamsEncodings = consumableParams["encodings"];
		auto encodingWithScalabilityModeIt = std::find_if(
			consumableParamsEncodings.begin(), consumableParamsEncodings.end(), [&](const json& encoding) {
				return encoding.count("scalabilityMode");
			});

		std::string scalabilityMode = encodingWithScalabilityModeIt != consumableParamsEncodings.end()
			? (*encodingWithScalabilityModeIt)["scalabilityMode"]
			: "";

		// If there is simulast, mangle spatial layers in scalabilityMode.
		if (consumableParams["encodings"].size() > 1)
		{
			int temporalLayers = parseScalabilityMode(scalabilityMode)["temporalLayers"];

			scalabilityMode = Utils::Printf("S%dT%d", consumableParams["encodings"].size(), temporalLayers);
		}

		if (!scalabilityMode.empty())
			consumerEncoding["scalabilityMode"] = scalabilityMode;

		// Use the maximum maxBitrate in any encoding and honor it in the Consumer's
		// encoding.
		uint32_t maxEncodingMaxBitrate = 0;
		for (auto& encoding : consumableParams["encodings"])
		{
			uint32_t maxBitrate = encoding.value("maxBitrate", 0);
			maxEncodingMaxBitrate = std::max(maxEncodingMaxBitrate, maxBitrate);
		}

		if (maxEncodingMaxBitrate)
		{
			consumerEncoding["maxBitrate"] = maxEncodingMaxBitrate;
		}

		// Set a single encoding for the Consumer.
		consumerParams["encodings"].push_back(consumerEncoding);
	}
	else
	{
		json consumableEncodings = Utils::clone(consumerParams["encodings"]);
		uint32_t baseSsrc = Utils::generateRandomNumber();
		uint32_t baseRtxSsrc = Utils::generateRandomNumber();

		for (int i = 0; i < consumableEncodings.size(); ++i)
		{
			json& encoding = consumableEncodings[i];

			encoding["ssrc"] = baseSsrc + i;

			if (rtxSupported)
				encoding["rtx"] = { {"ssrc", baseRtxSsrc + i}};
			else
				encoding.erase("rtx");

			consumerParams["encodings"].push_back(encoding);
		}
	}

	return consumerParams;
}

/**
 * Generate RTP parameters for a pipe Consumer.
 *
 * It keeps all original consumable encodings and removes support for BWE. If
 * enableRtx is false, it also removes RTX and NACK support.
 */
json getPipeConsumerRtpParameters(const json& consumableParams, bool enableRtx/* = false*/)
{
	json consumerParams =
	{
		{ "codecs", json::array() },
		{ "headerExtensions", json::array() },
		{ "encodings", json::array() },
		{ "rtcp", consumableParams["rtcp"] }
	};

	json consumableCodecs =
		Utils::clone(consumableParams["codecs"]);

	for (auto& codec : consumableCodecs)
	{
		if (!enableRtx && isRtxCodec(codec))
			continue;

		for (auto& fb : codec["rtcpFeedback"])
		{
			std::string type = fb.value("type", "");
			std::string parameter = fb.value("parameter", "");

			if ((type == "nack" && parameter == "pli") ||
				(type == "ccm" && parameter == "fir") ||
				(enableRtx && type == "nack" && parameter.empty())
				)
			{
				codec["rtcpFeedback"].push_back(fb);
			}
		}

		consumerParams["codecs"].push_back(codec);
	}

	// Reduce RTP extensions by disabling transport MID and BWE related ones.
	for (auto& ext : consumableParams["headerExtensions"])
	{
		std::string uri = ext["uri"];

		if (uri != "urn:ietf:params:rtp-hdrext:sdes:mid" &&
			uri != "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time" &&
			uri != "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01")
		{
			consumerParams["headerExtensions"].push_back(ext);
		}
	}

	json consumableEncodings = Utils::clone(consumableParams["encodings"]);

	for (auto& encoding : consumableEncodings)
	{
		if (!enableRtx)
			encoding.erase("rtx");

		consumerParams["encodings"].push_back(encoding);
	}

	return consumerParams;
}

} // namespace ortc

// Private helpers used in this file.

static bool isRtxCodec(const json& codec)
{
	MSC_TRACE();

	static const std::regex RtxMimeTypeRegex(
		"^(audio|video)/rtx$", std::regex_constants::ECMAScript | std::regex_constants::icase);

	std::smatch match;
	auto mimeType = codec["mimeType"].get<std::string>();

	return std::regex_match(mimeType, match, RtxMimeTypeRegex);
}

static bool matchCodecs(json& aCodec, json& bCodec, bool strict, bool modify)
{
	MSC_TRACE();

	auto aMimeTypeIt = aCodec.find("mimeType");
	auto bMimeTypeIt = bCodec.find("mimeType");
	auto aMimeType = aMimeTypeIt->get<std::string>();
	auto bMimeType = bMimeTypeIt->get<std::string>();

	std::transform(aMimeType.begin(), aMimeType.end(), aMimeType.begin(), ::tolower);
	std::transform(bMimeType.begin(), bMimeType.end(), bMimeType.begin(), ::tolower);

	if (aMimeType != bMimeType)
		return false;

	if (aCodec["clockRate"] != bCodec["clockRate"])
		return false;

	if (aCodec.contains("channels") != bCodec.contains("channels"))
		return false;

	if (aCodec.contains("channels") && aCodec["channels"] != bCodec["channels"])
		return false;

	// Per codec special checks.
	if (aMimeType == "audio/multiopus")
	{
		int aNumStreams = aCodec["parameters"]["num_streams"];
		int bNumStreams = bCodec["parameters"]["num_streams"];

		if (aNumStreams != bNumStreams)
			return false;

		int aCoupledStreams = aCodec["parameters"]["coupled_streams"];
		int bCoupledStreams = bCodec["parameters"]["coupled_streams"];

		if (aCoupledStreams != bCoupledStreams)
			return false;
	}
	else if (aMimeType == "video/h264" || aMimeType == "video/h264-svc")
	{
		// If strict matching check profile-level-id.
		if (strict)
		{
			auto aPacketizationMode = getH264PacketizationMode(aCodec);
			auto bPacketizationMode = getH264PacketizationMode(bCodec);

			if (aPacketizationMode != bPacketizationMode)
				return false;


			webrtc::SdpVideoFormat::Parameters aParameters;
			webrtc::SdpVideoFormat::Parameters bParameters;

			aParameters["level-asymmetry-allowed"] = std::to_string(getH264LevelAssimetryAllowed(aCodec));
			aParameters["packetization-mode"] = std::to_string(aPacketizationMode);
			aParameters["profile-level-id"] = getH264ProfileLevelId(aCodec);
			bParameters["level-asymmetry-allowed"] = std::to_string(getH264LevelAssimetryAllowed(bCodec));
			bParameters["packetization-mode"] = std::to_string(bPacketizationMode);
			bParameters["profile-level-id"] = getH264ProfileLevelId(bCodec);

			if (!webrtc::H264IsSameProfile(aParameters, bParameters))
				return false;

			webrtc::SdpVideoFormat::Parameters newParameters;

			try
			{
				webrtc::H264GenerateProfileLevelIdForAnswer(aParameters, bParameters, &newParameters);
			}
			catch (std::runtime_error)
			{
				return false;
			}

			if (modify)
			{
				auto profileLevelIdIt = newParameters.find("profile-level-id");

				if (profileLevelIdIt != newParameters.end())
				{
					aCodec["parameters"]["profile-level-id"] = profileLevelIdIt->second;
				}
				else
				{
					aCodec["parameters"].erase("profile-level-id");
				}
			}
		}
	}
	else if (aMimeType == "video/vp9")
	{
		// If strict matching check profile-id.
		if (strict)
		{
			auto aProfileId = getVP9ProfileId(aCodec);
			auto bProfileId = getVP9ProfileId(bCodec);

			if (aProfileId != bProfileId)
				return false;
		}
	}

	return true;
}

static uint8_t getH264PacketizationMode(const json& codec)
{
	MSC_TRACE();

	auto& parameters = codec["parameters"];
	auto packetizationModeIt = parameters.find("packetization-mode");

	// clang-format off
	if (
		packetizationModeIt == parameters.end() ||
		!packetizationModeIt->is_number_integer()
		)
		// clang-format on
	{
		return 0;
	}

	return packetizationModeIt->get<uint8_t>();
}

static uint8_t getH264LevelAssimetryAllowed(const json& codec)
{
	MSC_TRACE();

	auto& parameters = codec["parameters"];
	auto levelAssimetryAllowedIt = parameters.find("level-asymmetry-allowed");

	// clang-format off
	if (
		levelAssimetryAllowedIt == parameters.end() ||
		!levelAssimetryAllowedIt->is_number_integer()
		)
		// clang-format on
	{
		return 0;
	}

	return levelAssimetryAllowedIt->get<uint8_t>();
}

static std::string getH264ProfileLevelId(const json& codec)
{
	MSC_TRACE();

	auto& parameters = codec["parameters"];
	auto profileLevelIdIt = parameters.find("profile-level-id");

	if (profileLevelIdIt == parameters.end())
		return "";
	else if (profileLevelIdIt->is_number())
		return std::to_string(profileLevelIdIt->get<int32_t>());
	else
		return profileLevelIdIt->get<std::string>();
}

static std::string getVP9ProfileId(const json& codec)
{
	MSC_TRACE();

	auto& parameters = codec["parameters"];
	auto profileIdIt = parameters.find("profile-id");

	if (profileIdIt == parameters.end())
		return "0";

	if (profileIdIt->is_number())
		return std::to_string(profileIdIt->get<int32_t>());
	else
		return profileIdIt->get<std::string>();
}
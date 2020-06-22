#pragma once 

#include "common.hpp"
#include "ortc.hpp"
#include "utils.hpp"
#include "errors.hpp"
#include "supportedRtpCapabilities.hpp"
#include "scalabilityModes.hpp"
#include "RtpParameters.hpp"
#include "SctpParameters.hpp"

// type RtpMapping =
// {
// 	codecs:
// 	{
// 		payloadType: uint32_t;
// 		mappedPayloadType: uint32_t;
// 	}[];
// 
// 	encodings:
// 	{
// 		ssrc?: uint32_t;
// 		rid?: string;
// 		scalabilityMode?: string;
// 		mappedSsrc: uint32_t;
// 	}[];
// };
namespace ortc
{

	const std::vector<int> DynamicPayloadTypes =
	{
		100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
		111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
		122, 123, 124, 125, 126, 127, 96, 97, 98, 99
	};

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
	 * Validates RtpHeaderExtensionParameteters. It may modify given data by adding missing
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

		ortc::validateNumSctpStreams(*numStreamsIt);
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
		auto priorityIt = params.find("priority");
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

		// maxPacketLifeTime is optional. If unset set it to 0.
		if (maxPacketLifeTimeIt == params.end() || !maxPacketLifeTimeIt->is_number_integer())
		{
			params["maxPacketLifeTime"] = 0u;
		}

		// maxRetransmits is optional. If unset set it to 0.
		if (maxRetransmitsIt == params.end() || !maxRetransmitsIt->is_number_integer())
		{
			params["maxRetransmits"] = 0u;
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

		// priority is optional. If unset set it to empty string.
		if (priorityIt == params.end() || !priorityIt->is_string())
			params["priority"] = "";

		// label is optional. If unset set it to empty string.
		if (labelIt == params.end() || !labelIt->is_string())
			params["label"] = "";

		// protocol is optional. If unset set it to empty string.
		if (protocolIt == params.end() || !protocolIt->is_string())
			params["protocol"] = "";
	}


	/**
	 * Generate RTP capabilities for the Router based on the given media codecs and
	 * mediasoup supported RTP capabilities.
	 */
	json generateRouterRtpCapabilities(
		json& mediaCodecs/* = json::array()*/
	)
	{
		// Normalize supported RTP capabilities.
		validateRtpCapabilities(supportedRtpCapabilities);

		if (!Array.isArray(mediaCodecs))
			throw new TypeError("mediaCodecs must be an Array");

		const clonedSupportedRtpCapabilities =
			utils.clone(supportedRtpCapabilities) as RtpCapabilities;
		const dynamicPayloadTypes = utils.clone(DynamicPayloadTypes) as uint32_t[];
		const caps : RtpCapabilities =
		{
			codecs: [],
			headerExtensions : clonedSupportedRtpCapabilities.headerExtensions
		};

		for (const mediaCodec of mediaCodecs)
		{
			// This may throw.
			validateRtpCodecCapability(mediaCodec);

			const matchedSupportedCodec = clonedSupportedRtpCapabilities
				.codecs!
				.find((supportedCodec) = > (
					matchCodecs(mediaCodec, supportedCodec, { strict: false }))
				);

			if (!matchedSupportedCodec)
			{
				throw new UnsupportedError(
					`media codec not supported[mimeType:${ mediaCodec.mimeType }]`);
			}

			// Clone the supported codec.
			const codec = utils.clone(matchedSupportedCodec) as RtpCodecCapability;

			// If the given media codec has preferredPayloadType, keep it.
			if (typeof mediaCodec.preferredPayloadType == "uint32_t")
			{
				codec.preferredPayloadType = mediaCodec.preferredPayloadType;

				// Also remove the pt from the list of available dynamic values.
				const idx = dynamicPayloadTypes.indexOf(codec.preferredPayloadType);

				if (idx > -1)
					dynamicPayloadTypes.splice(idx, 1);
			}
			// Otherwise if the supported codec has preferredPayloadType, use it.
			else if (typeof codec.preferredPayloadType == "uint32_t")
			{
				// No need to remove it from the list since it"s not a dynamic value.
			}
			// Otherwise choose a dynamic one.
			else
			{
				// Take the first available pt and remove it from the list.
				const pt = dynamicPayloadTypes.shift();

				if (!pt)
					throw new Error("cannot allocate more dynamic codec payload types");

				codec.preferredPayloadType = pt;
			}

			// Ensure there is not duplicated preferredPayloadType values.
			if (caps.codecs!.some((c) = > c.preferredPayloadType == codec.preferredPayloadType))
				throw new TypeError("duplicated codec.preferredPayloadType");

			// Merge the media codec parameters.
			codec.parameters = { ...codec.parameters, ...mediaCodec.parameters };

			// Append to the codec list.
			caps.codecs!.push(codec);

			// Add a RTX video codec if video.
			if (codec.kind == "video")
			{
				// Take the first available pt and remove it from the list.
				const pt = dynamicPayloadTypes.shift();

				if (!pt)
					throw new Error("cannot allocate more dynamic codec payload types");

				const rtxCodec : RtpCodecCapability =
				{
					kind: codec.kind,
					mimeType : `${codec.kind } / rtx`,
							   preferredPayloadType : pt,
					clockRate : codec.clockRate,
					parameters :
				{
				apt: codec.preferredPayloadType
				},
					rtcpFeedback : []
			};

			// Append to the codec list.
			caps.codecs!.push(rtxCodec);
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
json getProducerRtpParametersMapping(
	json& params,
	json& caps
)
{
	const rtpMapping : RtpMapping =
	{
		codecs: [],
		encodings : []
	};

	// Match parameters media codecs to capabilities media codecs.
	const codecToCapCodec : Map<RtpCodecParameters, RtpCodecCapability> = new Map();

	for (const codec of params.codecs)
	{
		if (isRtxCodec(codec))
			continue;

		// Search for the same media codec in capabilities.
		const matchedCapCodec = caps.codecs!
			.find((capCodec) = > (
				matchCodecs(codec, capCodec, { strict: true, modify : true }))
			);

		if (!matchedCapCodec)
		{
			throw new UnsupportedError(
				`unsupported codec[mimeType:${ codec.mimeType }, payloadType : ${ codec.payloadType }]`);
		}

		codecToCapCodec.set(codec, matchedCapCodec);
	}

	// Match parameters RTX codecs to capabilities RTX codecs.
	for (const codec of params.codecs)
	{
		if (!isRtxCodec(codec))
			continue;

		// Search for the associated media codec.
		const associatedMediaCodec = params.codecs
			.find((mediaCodec) = > mediaCodec.payloadType == codec.parameters.apt);

		if (!associatedMediaCodec)
		{
			throw new TypeError(
				`missing media codec found for RTX PT ${ codec.payloadType }`);
		}

		const capMediaCodec = codecToCapCodec.get(associatedMediaCodec);

		// Ensure that the capabilities media codec has a RTX codec.
		const associatedCapRtxCodec = caps.codecs!
			.find((capCodec) = > (
				isRtxCodec(capCodec) &&
				capCodec.parameters.apt == capMediaCodec!.preferredPayloadType
				));

		if (!associatedCapRtxCodec)
		{
			throw new UnsupportedError(
				`no RTX codec for capability codec PT ${ capMediaCodec!.preferredPayloadType }`);
		}

		codecToCapCodec.set(codec, associatedCapRtxCodec);
	}

	// Generate codecs mapping.
	for (const[codec, capCodec] of codecToCapCodec)
	{
		rtpMapping.codecs.push(
			{
				payloadType: codec.payloadType,
				mappedPayloadType : capCodec.preferredPayloadType!
			});
	}

	// Generate encodings mapping.
	let mappedSsrc = utils.generateRandomNumber();

	for (const encoding of params.encodings!)
	{
		const mappedEncoding : any = {};

		mappedEncoding.mappedSsrc = mappedSsrc++;

		if (encoding.rid)
			mappedEncoding.rid = encoding.rid;
		if (encoding.ssrc)
			mappedEncoding.ssrc = encoding.ssrc;
		if (encoding.scalabilityMode)
			mappedEncoding.scalabilityMode = encoding.scalabilityMode;

		rtpMapping.encodings.push(mappedEncoding);
	}

	return rtpMapping;
}

/**
 * Generate RTP parameters to be internally used by Consumers given the RTP
 * parameters of a Producer and the RTP capabilities of the Router.
 */
json getConsumableRtpParameters(
	std::string kind,
	json& params,
	json& caps,
	json& rtpMapping
)
{
	const consumableParams : RtpParameters =
	{
		codecs: [],
		headerExtensions : [],
		encodings : [],
		rtcp : {}
	};

	for (const codec of params.codecs)
	{
		if (isRtxCodec(codec))
			continue;

		const consumableCodecPt = rtpMapping.codecs
			.find((entry) = > entry.payloadType == codec.payloadType)!
			.mappedPayloadType;

		const matchedCapCodec = caps.codecs!
			.find((capCodec) = > capCodec.preferredPayloadType == consumableCodecPt)!;

		const consumableCodec : RtpCodecParameters =
		{
			mimeType: matchedCapCodec.mimeType,
			payloadType : matchedCapCodec.preferredPayloadType!,
			clockRate : matchedCapCodec.clockRate,
			channels : matchedCapCodec.channels,
			parameters : codec.parameters, // Keep the Producer codec parameters.
			rtcpFeedback : matchedCapCodec.rtcpFeedback
		};

		consumableParams.codecs.push(consumableCodec);

		const consumableCapRtxCodec = caps.codecs!
			.find((capRtxCodec) = > (
				isRtxCodec(capRtxCodec) &&
				capRtxCodec.parameters.apt == consumableCodec.payloadType
				));

		if (consumableCapRtxCodec)
		{
			const consumableRtxCodec : RtpCodecParameters =
			{
				mimeType: consumableCapRtxCodec.mimeType,
				payloadType : consumableCapRtxCodec.preferredPayloadType!,
				clockRate : consumableCapRtxCodec.clockRate,
				parameters : consumableCapRtxCodec.parameters,
				rtcpFeedback : consumableCapRtxCodec.rtcpFeedback
			};

			consumableParams.codecs.push(consumableRtxCodec);
		}
	}

	for (const capExt of caps.headerExtensions!)
	{

		// Just take RTP header extension that can be used in Consumers.
		if (
			capExt.kind != = kind ||
			(capExt.direction != = "sendrecv" && capExt.direction != = "sendonly")
			)
		{
			continue;
		}

		const consumableExt =
		{
			uri: capExt.uri,
			id : capExt.preferredId,
			encrypt : capExt.preferredEncrypt,
			parameters : {}
		};

		consumableParams.headerExtensions!.push(consumableExt);
	}

	// Clone Producer encodings since we"ll mangle them.
	const consumableEncodings = utils.clone(params.encodings) as RtpEncodingParameters[];

	for (let i = 0; i < consumableEncodings.length; ++i)
	{
		const consumableEncoding = consumableEncodings[i];
		const { mappedSsrc } = rtpMapping.encodings[i];

		// Remove useless fields.
		delete consumableEncoding.rid;
		delete consumableEncoding.rtx;
		delete consumableEncoding.codecPayloadType;

		// Set the mapped ssrc.
		consumableEncoding.ssrc = mappedSsrc;

		consumableParams.encodings!.push(consumableEncoding);
	}

	consumableParams.rtcp =
	{
		cname: params.rtcp!.cname,
		reducedSize : true,
		mux : true
	};

	return consumableParams;
}

/**
 * Check whether the given RTP capabilities can consume the given Producer.
 */
bool canConsume(
	json& consumableParams,
	json& caps
)
{
	// This may throw.
	validateRtpCapabilities(caps);

	const matchingCodecs : RtpCodecParameters[] = [];

	for (const codec of consumableParams.codecs)
	{
		const matchedCapCodec = caps.codecs!
			.find((capCodec) = > matchCodecs(capCodec, codec, { strict: true }));

		if (!matchedCapCodec)
			continue;

		matchingCodecs.push(codec);
	}

	// Ensure there is at least one media codec.
	if (matchingCodecs.length == 0 || isRtxCodec(matchingCodecs[0]))
		return false;

	return true;
}

/**
 * Generate RTP parameters for a specific Consumer.
 *
 * It reduces encodings to just one and takes into account given RTP capabilities
 * to reduce codecs, codecs" RTCP feedback and header extensions, and also enables
 * or disabled RTX.
 */
json  getConsumerRtpParameters(
	json& consumableParams,
	json& caps
)
{
	const consumerParams : RtpParameters =
	{
		codecs: [],
		headerExtensions : [],
		encodings : [],
		rtcp : consumableParams.rtcp
	};

	for (const capCodec of caps.codecs!)
	{
		validateRtpCodecCapability(capCodec);
	}

	const consumableCodecs =
		utils.clone(consumableParams.codecs) as RtpCodecParameters[];

	let rtxSupported = false;

	for (const codec of consumableCodecs)
	{
		const matchedCapCodec = caps.codecs!
			.find((capCodec) = > matchCodecs(capCodec, codec, { strict: true }));

		if (!matchedCapCodec)
			continue;

		codec.rtcpFeedback = matchedCapCodec.rtcpFeedback;

		consumerParams.codecs.push(codec);

		if (!rtxSupported && isRtxCodec(codec))
			rtxSupported = true;
	}

	// Ensure there is at least one media codec.
	if (consumerParams.codecs.length == 0 || isRtxCodec(consumerParams.codecs[0]))
	{
		throw new UnsupportedError("no compatible media codecs");
	}

	consumerParams.headerExtensions = consumableParams.headerExtensions!
		.filter((ext) = > (
			caps.headerExtensions!
			.some((capExt) = > (
				capExt.preferredId == ext.id &&
				capExt.uri == ext.uri
				))
			));

	// Reduce codecs" RTCP feedback. Use Transport-CC if available, REMB otherwise.
	if (
		consumerParams.headerExtensions.some((ext) = > (
			ext.uri == "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01"
			))
		)
	{
		for (const codec of consumerParams.codecs)
		{
			codec.rtcpFeedback = codec.rtcpFeedback!
				.filter((fb) = > fb.type != = "goog-remb");
		}
	}
	else if (
		consumerParams.headerExtensions.some((ext) = > (
			ext.uri == "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time"
			))
		)
	{
		for (const codec of consumerParams.codecs)
		{
			codec.rtcpFeedback = codec.rtcpFeedback!
				.filter((fb) = > fb.type != = "transport-cc");
		}
	}
	else
	{
		for (const codec of consumerParams.codecs)
		{
			codec.rtcpFeedback = codec.rtcpFeedback!
				.filter((fb) = > (
					fb.type != = "transport-cc" &&
					fb.type != = "goog-remb"
					));
		}
	}

	const consumerEncoding : RtpEncodingParameters =
	{
		ssrc: utils.generateRandomNumber()
	};

	if (rtxSupported)
		consumerEncoding.rtx = { ssrc: utils.generateRandomNumber() };

	// If any of the consumableParams.encodings has scalabilityMode, process it
	// (assume all encodings have the same value).
	const encodingWithScalabilityMode =
		consumableParams.encodings!.find((encoding) = > encoding.scalabilityMode);

	let scalabilityMode = encodingWithScalabilityMode
		? encodingWithScalabilityMode.scalabilityMode
		: undefined;

	// If there is simulast, mangle spatial layers in scalabilityMode.
	if (consumableParams.encodings!.length > 1)
	{
		const { temporalLayers } = parseScalabilityMode(scalabilityMode);

		scalabilityMode = `S${consumableParams.encodings!.length
	}T${ temporalLayers }`;
}

if (scalabilityMode)
consumerEncoding.scalabilityMode = scalabilityMode;

// Use the maximum maxBitrate in any encoding and honor it in the Consumer"s
// encoding.
const maxEncodingMaxBitrate =
consumableParams.encodings!.reduce((maxBitrate, encoding) = > (
	encoding.maxBitrate && encoding.maxBitrate > maxBitrate
	? encoding.maxBitrate
	: maxBitrate
	), 0);

if (maxEncodingMaxBitrate)
{
	consumerEncoding.maxBitrate = maxEncodingMaxBitrate;
}

// Set a single encoding for the Consumer.
consumerParams.encodings!.push(consumerEncoding);

// Copy verbatim.
consumerParams.rtcp = consumableParams.rtcp;

return consumerParams;
}

/**
 * Generate RTP parameters for a pipe Consumer.
 *
 * It keeps all original consumable encodings and removes support for BWE. If
 * enableRtx is false, it also removes RTX and NACK support.
 */
json getPipeConsumerRtpParameters(
	json& consumableParams,
	bool enableRtx = false
)
{
	const consumerParams : RtpParameters =
	{
		codecs: [],
		headerExtensions : [],
		encodings : [],
		rtcp : consumableParams.rtcp
	};

	const consumableCodecs =
		utils.clone(consumableParams.codecs) as RtpCodecParameters[];

	for (const codec of consumableCodecs)
	{
		if (!enableRtx && isRtxCodec(codec))
			continue;

		codec.rtcpFeedback = codec.rtcpFeedback!
			.filter((fb) = > (
			(fb.type == "nack" && fb.parameter == "pli") ||
				(fb.type == "ccm" && fb.parameter == "fir") ||
				(enableRtx && fb.type == "nack" && !fb.parameter)
				));

		consumerParams.codecs.push(codec);
	}

	// Reduce RTP extensions by disabling transport MID and BWE related ones.
	consumerParams.headerExtensions = consumableParams.headerExtensions!
		.filter((ext) = > (
			ext.uri != = "urn:ietf:params:rtp-hdrext:sdes:mid" &&
			ext.uri != = "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time" &&
			ext.uri != = "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01"
			));

	const consumableEncodings =
		utils.clone(consumableParams.encodings) as RtpEncodingParameters[];

	for (const encoding of consumableEncodings)
	{
		if (!enableRtx)
			delete encoding.rtx;

		consumerParams.encodings!.push(encoding);
	}

	return consumerParams;
}

bool isRtxCodec(json& codec)
{
	return / . + \ / rtx$ / i.test(codec.mimeType);
}

bool matchCodecs(json& aCodec, json&bCodec, bool strict/* = false*/, bool modify/* = false*/)
{
	const aMimeType = aCodec.mimeType.toLowerCase();
	const bMimeType = bCodec.mimeType.toLowerCase();

	if (aMimeType != = bMimeType)
		return false;

	if (aCodec.clockRate != = bCodec.clockRate)
		return false;

	if (aCodec.channels != = bCodec.channels)
		return false;

	// Per codec special checks.
	switch (aMimeType)
	{
	case "video/h264":
	{
		const aPacketizationMode = aCodec.parameters["packetization-mode"] || 0;
		const bPacketizationMode = bCodec.parameters["packetization-mode"] || 0;

		if (aPacketizationMode != = bPacketizationMode)
			return false;

		// If strict matching check profile-level-id.
		if (strict)
		{
			if (!h264.isSameProfile(aCodec.parameters, bCodec.parameters))
				return false;

			let selectedProfileLevelId;

			try
			{
				selectedProfileLevelId =
					h264.generateProfileLevelIdForAnswer(aCodec.parameters, bCodec.parameters);
			}
			catch (error)
			{
				return false;
			}

			if (modify)
			{
				if (selectedProfileLevelId)
					aCodec.parameters["profile-level-id"] = selectedProfileLevelId;
				else
					delete aCodec.parameters["profile-level-id"];
			}
		}

		break;
	}

	case "video/vp9":
	{
		// If strict matching check profile-id.
		if (strict)
		{
			const aProfileId = aCodec.parameters["profile-id"] || 0;
			const bProfileId = bCodec.parameters["profile-id"] || 0;

			if (aProfileId != = bProfileId)
				return false;
		}

		break;
	}
	}

	return true;
}
}

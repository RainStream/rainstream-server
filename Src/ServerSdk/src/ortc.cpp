#include "RainStream.hpp"
#include "ortc.hpp"
#include "supportedRtpCapabilities.hpp"
#include "errors.hpp"
#include "utils.hpp"

namespace Object
{
	Json assign(Json target, Json source)
	{
		if (source.is_object())
		{
			for (Json::iterator it = source.begin(); it != source.end(); ++it) 
			{
				target[it.key()] = it.value();
			}
		}
		
		return target;
	}
}

namespace ortc
{
	const std::vector<int> DYNAMIC_PAYLOAD_TYPES =
	{
		100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
		116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 96, 97, 98, 99, 77,
		78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 35, 36,
		37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
		57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71
	};

	Json generateRoomRtpCapabilities(Json mediaCodecs)
	{
		if (!mediaCodecs.is_array())
			throw rs::TypeError("mediaCodecs must be an Array");
		else if (mediaCodecs.size() == 0)
			throw rs::TypeError("mediaCodecs cannot be empty");

		const Json supportedCodecs = supportedRtpCapabilities["codecs"];
		Json caps =
		{
			{ "codecs", Json::array()},
			{ "headerExtensions" , supportedRtpCapabilities["headerExtensions"] },
			{ "fecMechanisms" , supportedRtpCapabilities["fecMechanisms"] }
		};

		uint32_t dynamicPayloadTypeIdx = 0;

		for (auto& mediaCodec : mediaCodecs)
		{
			assertCodecCapability(mediaCodec);

			Json matchedSupportedCodec;

			for (auto& supportedCodec : supportedCodecs)
			{
				if (matchCodecs(mediaCodec, supportedCodec))
				{
					matchedSupportedCodec = supportedCodec;
				}
			}

			if (matchedSupportedCodec.is_null())
				throw rs::Error("RoomMediaCodec not supported[name:${ mediaCodec.name }]");

			// Clone the supported codec.
			Json codec = matchedSupportedCodec;

			// Normalize channels.
			if (codec["kind"] != "audio")
				codec.erase("channels");
			else if (!codec.count("channels"))
				codec["channels"] = 1;

			// Merge the media codec parameters.
			codec["parameters"] = Object::assign(codec["parameters"], mediaCodec["parameters"]);

			// Ensure rtcpFeedback is an Array.
			if (!codec.count("rtcpFeedback"))
			{
				codec["rtcpFeedback"] = Json::array();
			}

			// Assign a payload type.
			if (!codec["preferredPayloadType"].is_number())
			{
				int pt = DYNAMIC_PAYLOAD_TYPES[dynamicPayloadTypeIdx++];

				if (!pt)
					throw rs::Error("cannot allocate more dynamic codec payload types");

				codec["preferredPayloadType"] = pt;
			}

			// Append to the codec list.
			caps["codecs"].push_back(codec);

			// Add a RTX video codec if video.
			if (codec["kind"] == "video")
			{
				const int pt = DYNAMIC_PAYLOAD_TYPES[dynamicPayloadTypeIdx++];

				if (!pt)
					throw rs::Error("cannot allocate more dynamic codec payload types");

				Json rtxCodec =
				{
					{ "kind", codec["kind"] },
					{ "name" , "rtx" },
					{ "mimeType" , codec["kind"].get<std::string>() + "/rtx" },
					{ "preferredPayloadType" , pt },
					{ "clockRate" , codec["clockRate"] },
					{ "parameters" ,
						{
							{ "apt", codec["preferredPayloadType"] }
						}
					}
				};

				// Append to the codec list.
				caps["codecs"].push_back(rtxCodec);
			}
		}

		return caps;
	}

	bool assertCapabilitiesSubset(Json aCaps, Json bCaps)
	{
		for (auto& aCodec : aCaps["codecs"])
		{
			Json matchedBCodec;
			for (auto& bCodec : bCaps["codecs"])
			{
				if (aCodec["preferredPayloadType"] == bCodec["preferredPayloadType"] &&
					matchCodecs(aCodec, bCodec))
					matchedBCodec.push_back(bCodec);
			}

			if (!matchedBCodec.size())
			{
				throw rs::Error(
					"unsupported codec " +
					utils::Printf("[name:${ aCodec.name }, preferredPayloadType : ${ aCodec.preferredPayloadType }]"));
			}
		}

		for (auto& aExt : aCaps["headerExtensions"])
		{
			Json matchedBExt;
			for (auto& bExt : bCaps["headerExtensions"])
			{
				if (aExt["preferredId"] == bExt["preferredId"] &&
					matchHeaderExtensions(aExt, bExt))
					matchedBExt.push_back(bExt);
			}

			if (!matchedBExt.size())
			{
				throw rs::Error(
					"unsupported header extension[kind:${ aExt.kind }, uri : ${ aExt.uri }]");
			}
		}
	};

	Json getProducerRtpParametersMapping(Json params, Json caps)
	{
		std::map<Json,Json> codecToCapCodec;

		// Match parameters media codecs to capabilities media codecs.
		for (auto& codec : params["codecs"])
		{
			std::string codec_name = codec["name"].get<std::string>();

			if (utils::ToLowerCase(codec_name) == "rtx")
				continue;

			// Search for the same media codec in capabilities.
			Json matchedCapCodec;
			for (auto& capCodec : caps["codecs"])
			{
				if (matchCodecs(codec, capCodec))
				{
					matchedCapCodec = capCodec;
					break;
				}
			}

			if (matchedCapCodec.is_null())
			{
				throw rs::Error(
					"unsupported codec " +
					utils::Printf("[name:%s, payloadType : %d]", 
						codec["name"].get<std::string>().c_str(),
						codec["payloadType"].get<int>()));
			}

			codecToCapCodec[codec] = matchedCapCodec;
		}

		// Match parameters RTX codecs to capabilities RTX codecs.
		for (auto& codec : params["codecs"])
		{
			std::string codec_name = codec["name"].get<std::string>();

			if (utils::ToLowerCase(codec_name) != "rtx")
				continue;

			// Search for the associated media codec in parameters.
			Json associatedMediaCodec;

			for (auto& mediaCodec : params["codecs"])
			{
				if (mediaCodec["payloadType"].get<int>() == codec["parameters"]["apt"].get<int>())
				{
					associatedMediaCodec = mediaCodec;
					break;
				}
			}

			Json capMediaCodec = codecToCapCodec[associatedMediaCodec];

			if (capMediaCodec.is_null())
			{
				throw rs::Error(utils::Printf("no media codec found for RTX PT %d",
					codec["payloadType"].get<int>()));
			}

			// Ensure that the capabilities media codec has a RTX codec.
			Json associatedCapRtxCodec;
			for (auto& capCodec : caps["codecs"])
			{
				if (utils::ToLowerCase(capCodec["name"].get<std::string>()) == "rtx" &&
					capCodec["parameters"]["apt"].get<int>() == capMediaCodec["preferredPayloadType"].get<int>())
				{
					associatedCapRtxCodec = capCodec;
					break;
				}
			}

			if (associatedCapRtxCodec.is_null())
			{
				throw rs::Error(
					"no RTX codec found in capabilities " +
					utils::Printf("[capabilities codec PT : %d]", capMediaCodec["preferredPayloadType"].get<int>()));
			}

			codecToCapCodec[codec] = associatedCapRtxCodec;
		}

		std::map<int, int> mapCodecPayloadTypes;

		for (auto it : codecToCapCodec)
		{
			Json codec = it.first;
			Json capCodec = it.second;
			int payloadType = codec["payloadType"].get<int>();
			int preferredPayloadType = capCodec["preferredPayloadType"].get<int>();

			mapCodecPayloadTypes[payloadType] = preferredPayloadType;
		}

		std::map<int, int>  mapHeaderExtensionIds;

		for (auto& ext : params["headerExtensions"])
		{
			Json matchedCapExt;
			for (auto capExt : caps["headerExtensions"])
			{
				if (matchHeaderExtensions(ext, capExt))
				{
					matchedCapExt = capExt;
					break;
				}
			}

			if (matchedCapExt.is_null())
			{
				throw rs::Error(
					"unsupported header extensions[uri:\"${ext.uri}\", id : ${ ext.id }]");
			}

			mapHeaderExtensionIds[ext["id"].get<int>()] = matchedCapExt["preferredId"].get<int>();
		}

		return Json{
			{ "mapCodecPayloadTypes" , mapCodecPayloadTypes },
			{ "mapHeaderExtensionIds" , mapHeaderExtensionIds }
		};
	};

	Json getConsumableRtpParameters(Json params, Json caps, Json rtpMapping)
	{
		std::map<int, int> mapCodecPayloadTypes = rtpMapping["mapCodecPayloadTypes"].get<std::map<int,int>>();
		std::map<int, int> mapHeaderExtensionIds = rtpMapping["mapHeaderExtensionIds"].get<std::map<int, int>>();
		
		Json consumableParams =
		{
			{ "muxId" , params["muxId"]},
			{ "codecs" , Json::array() },
			{ "headerExtensions" , Json::array() },
			{ "encodings" , params["encodings"] },
			{ "rtcp" , params["rtcp"] }
		};

		for (auto& codec : params["codecs"])
		{
			std::string codec_name = codec["name"].get<std::string>();
			if (utils::ToLowerCase(codec_name) == "rtx")
				continue;

			int payloadType = codec["payloadType"].get<int>();
			int consumableCodecPt = mapCodecPayloadTypes[payloadType];

			if (!consumableCodecPt)
			{
				throw rs::Error(
					"codec payloadType mapping not found[name:${ codec_name }]");
			}

			Json matchedCapCodec;
			for (auto capCodec : caps["codecs"])
			{
				if (capCodec["preferredPayloadType"] == consumableCodecPt)
				{
					matchedCapCodec = capCodec;
					break;
				}
			}

			if (matchedCapCodec.is_null())
				throw rs::Error("capabilities codec not found[name:${ codec_name }]");

			Json consumableCodec =
			{
				{ "name", matchedCapCodec["name"]},
				{ "mimeType" , matchedCapCodec["mimeType"] },
				{ "clockRate", matchedCapCodec["clockRate"] },
				{ "payloadType" , matchedCapCodec["preferredPayloadType"] },
				{ "channels" , matchedCapCodec.value("channels",0) },
				{ "rtcpFeedback" , matchedCapCodec["rtcpFeedback"] },
				{ "parameters", matchedCapCodec["parameters"] }
			};

			if (!consumableCodec.value("channels",0))
				consumableCodec.erase("channels");

			consumableParams["codecs"].push_back(consumableCodec);

			Json consumableCapRtxCodec;
			for (auto& capRtxCodec : caps["codecs"])
			{
				if (utils::ToLowerCase(capRtxCodec["name"].get<std::string>()) == "rtx" &&
					capRtxCodec["parameters"]["apt"].get<int>() == consumableCodec["payloadType"].get<int>())
				{
					consumableCapRtxCodec = capRtxCodec;
					break;
				}
			}

			if (!consumableCapRtxCodec.is_null())
			{
				Json consumableRtxCodec =
				{
					{ "name" , consumableCapRtxCodec["name"]},
					{ "mimeType" , consumableCapRtxCodec["mimeType"] },
					{ "clockRate" , consumableCapRtxCodec["clockRate"] },
					{ "payloadType" , consumableCapRtxCodec["preferredPayloadType"] },
					{ "channels" , consumableCapRtxCodec.value("channels",0) },
					{ "rtcpFeedback" , consumableCapRtxCodec["rtcpFeedback"] },
					{ "parameters" , consumableCapRtxCodec["parameters"] }
				};

				if (!consumableRtxCodec.value("channels", 0))
					consumableRtxCodec.erase("channels");

				consumableParams["codecs"].push_back(consumableRtxCodec);
			}
		}

		for (auto& ext : params["headerExtensions"])
		{
			int consumableExtId = mapHeaderExtensionIds[ext["id"].get<int>()];

			if (!consumableExtId)
			{
				throw rs::Error(
					"extension header id mapping not found[uri:${ ext.uri }]");
			}

			Json matchedCapExt;
			for (auto& capExt : caps["headerExtensions"])
			{
				if (capExt["preferredId"].get<int>() == consumableExtId)
				{
					matchedCapExt = capExt;
					break;
				}
			}

			if (matchedCapExt.is_null())
				throw rs::Error("capabilities header extension not found[uri:${ ext.uri }]");

			Json consumableExt =
			{
				{ "uri" , matchedCapExt["uri"]},
				{ "id " , matchedCapExt["preferredId"]}
			};

			consumableParams["headerExtensions"].push_back(consumableExt);
		}

		return consumableParams;
	};

	Json getConsumerRtpParameters(Json consumableRtpParameters, Json rtpCapabilities)
	{
		Json consumerParams =
		{
			{ "muxId" , consumableRtpParameters["muxId"] },
			{ "codecs" ,Json::array() },
			{ "headerExtensions" , Json::array() },
			{ "encodings" , Json::array() },
			{ "rtcp" , consumableRtpParameters["rtcp"] }
		};

		Json consumableCodecs = utils::cloneObject(consumableRtpParameters["codecs"]);
		bool rtxSupported = false;

		for (auto codec : consumableCodecs)
		{
			int payloadType = payloadType = codec["payloadType"].get<int>();
			
			Json matchedCapCodec;
			
			for (auto capCodec : rtpCapabilities["codecs"])
			{
				int preferredPayloadType = capCodec["preferredPayloadType"].get<int>();
				
				if (preferredPayloadType == payloadType)
				{
					matchedCapCodec = capCodec;
					break;
				}
			}

			if (matchedCapCodec.is_null())
				continue;

			Json newcodec = codec;

			// Reduce RTCP feedbacks.
			newcodec["rtcpFeedback"] = matchedCapCodec["rtcpFeedback"];

			consumerParams["codecs"].push_back(newcodec);

			if (!rtxSupported && utils::ToLowerCase(newcodec["name"].get<std::string>()) == "rtx")
				rtxSupported = true;
		}

		for (auto ext : consumableRtpParameters["headerExtensions"])
		{
			for (auto& capExt : rtpCapabilities["headerExtensions"])
			{
				if (capExt["preferredId"] == ext["id"])
				{
					consumerParams["headerExtensions"].push_back(capExt);
				}
			}
		}

		Json consumerEncoding =
		{
			{ "ssrc" , utils::randomNumber() }
		};

		if (rtxSupported)
		{
			consumerEncoding["rtx"] =
			{
				{"ssrc" , utils::randomNumber()}
			};
		}

		consumerParams["encodings"].push_back(consumerEncoding);

		return consumerParams;
	};

	void assertCodecCapability(Json codec)
	{
		const bool valid =
			(codec.is_object() && !codec.is_array()) &&
			(codec["kind"].is_string() && !codec["kind"].get<std::string>().empty()) &&
			(codec["name"].is_string() && !codec["name"].get<std::string>().empty()) &&
			(codec["clockRate"].is_number() && codec["clockRate"].get<int>());

		if (!valid)
			throw rs::TypeError("invalid RTCCodecCapability");
	}

	bool matchCodecs(Json aCodec, Json bCodec)
	{
		std::string aName = utils::ToLowerCase(aCodec["name"].get<std::string>());
		std::string bName = utils::ToLowerCase(bCodec["name"].get<std::string>());

		if (aCodec.count("kind") && bCodec.count("kind") && 
			aCodec["kind"].get<std::string>() != bCodec["kind"].get<std::string>())
		{
			return false;
		}

		if (aName != bName)
			return false;

		if (aCodec.value("clockRate", 0) != bCodec.value("clockRate", 0))
			return false;

		if (aCodec.value("channels", 0) != bCodec.value("channels", 0))
			return false;

		// Match H264 parameters.
		if (aName == "h264")
		{
			int aPacketizationMode = aCodec["parameters"].value("packetization-mode", 0);
			int bPacketizationMode = bCodec["parameters"].value("packetization-mode", 0);

			if (aPacketizationMode != bPacketizationMode)
				return false;
		}

		return true;
	}

	bool matchHeaderExtensions(Json aExt, Json bExt)
	{
		if (aExt.count("kind") && bExt.count("kind") && aExt["kind"] != bExt["kind"])
			return false;

		if (aExt["uri"] != bExt["uri"])
			return false;

		return true;
	}
}

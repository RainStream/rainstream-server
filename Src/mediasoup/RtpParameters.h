#pragma once 

#include "common.h"

// /**
//  * The RTP capabilities define what mediasoup or an endpoint can receive at
//  * media level.
//  */
// struct RtpCapabilities
// {
// 	/**
// 	 * Supported media and RTX codecs.
// 	 */
// 	std::vector<RtpCodecCapability> codecs;
// 
// 	/**
// 	 * Supported RTP header extensions.
// 	 */
// 	std::vector<RtpHeaderExtension> headerExtensions;
// 
// 	/**
// 	 * Supported FEC mechanisms.
// 	 */
// 	std::vector<std::string> fecMechanisms;;
// }
// 
// /**
//  * Media kind ("audio" or "video").
//  */
// struct MediaKind = "audio" | "video";
// 
// /**
//  * Provides information on the capabilities of a codec within the RTP
//  * capabilities. The list of media codecs supported by mediasoup and their
//  * settings is defined in the supportedRtpCapabilities.ts file.
//  *
//  * Exactly one RtpCodecCapability will be present for each supported combination
//  * of parameters that requires a distinct value of preferredPayloadType. For
//  * example:
//  *
//  * - Multiple H264 codecs, each with their own distinct "packetization-mode" and
//  *   "profile-level-id" values.
//  * - Multiple VP9 codecs, each with their own distinct "profile-id" value.
//  *
//  * RtpCodecCapability entries in the mediaCodecs array of RouterOptions do not
//  * require preferredPayloadType field (if unset, mediasoup will choose a random
//  * one). If given, make sure it"s in the 96-127 range.
//  */
// struct RtpCodecCapability
// {
// 	/**
// 	 * Media kind.
// 	 */
// 	kind: MediaKind;
// 
// 	/**
// 	 * The codec MIME media type/subtype (e.g. "audio/opus", "video/VP8").
// 	 */
// 	std::string mimeType;
// 
// 	/**
// 	 * The preferred RTP payload type.
// 	 */
// 	uint32_t preferredPayloadType;
// 
// 	/**
// 	 * Codec clock rate expressed in Hertz.
// 	 */
// 	uint32_t clockRate;
// 
// 	/**
// 	 * The uint32_t of channels supported (e.g. two for stereo). Just for audio.
// 	 * Default 1.
// 	 */
// 	uint32_t channels;
// 
// 	/**
// 	 * Codec specific parameters. Some parameters (such as "packetization-mode"
// 	 * and "profile-level-id" in H264 or "profile-id" in VP9) are critical for
// 	 * codec matching.
// 	 */
// 	json parameters;
// 
// 	/**
// 	 * Transport layer and codec-specific feedback messages for this codec.
// 	 */
// 	std::vector<RtcpFeedback> rtcpFeedback;
// };
// 
// /**
//  * Direction of RTP header extension.
//  */
// struct RtpHeaderExtensionDirection = "sendrecv" | "sendonly" | "recvonly" | "inactive";
// 
// /**
//  * Provides information relating to supported header extensions. The list of
//  * RTP header extensions supported by mediasoup is defined in the
//  * supportedRtpCapabilities.ts file.
//  *
//  * mediasoup does not currently support encrypted RTP header extensions. The
//  * direction field is just present in mediasoup RTP capabilities (retrieved via
//  * router.rtpCapabilities or mediasoup.getSupportedRtpCapabilities()). It"s
//  * ignored if present in endpoints" RTP capabilities.
//  */
// struct RtpHeaderExtension
// {
// 	/**
// 	 * Media kind. If empty string, it"s valid for all kinds.
// 	 * Default any media kind.
// 	 */
// 	kind?: MediaKind | "";
// 
// 	/*
// 	 * The URI of the RTP header extension, as defined in RFC 5285.
// 	 */
// 	std::string uri;
// 
// 	/**
// 	 * The preferred numeric identifier that goes in the RTP packet. Must be
// 	 * unique.
// 	 */
// 	uint32_t preferredId;
// 
// 	/**
// 	 * If true, it is preferred that the value in the header be encrypted as per
// 	 * RFC 6904. Default false.
// 	 */
// 	bool preferredEncrypt;
// 
// 	/**
// 	 * If "sendrecv", mediasoup supports sending and receiving this RTP extension.
// 	 * "sendonly" means that mediasoup can send (but not receive) it. "recvonly"
// 	 * means that mediasoup can receive (but not send) it.
// 	 */
// 	direction?: RtpHeaderExtensionDirection;
// }
// 
// /**
//  * The RTP send parameters describe a media stream received by mediasoup from
//  * an endpoint through its corresponding mediasoup Producer. These parameters
//  * may include a mid value that the mediasoup transport will use to match
//  * received RTP packets based on their MID RTP extension value.
//  *
//  * mediasoup allows RTP send parameters with a single encoding and with multiple
//  * encodings (simulcast). In the latter case, each entry in the encodings array
//  * must include a ssrc field or a rid field (the RID RTP extension value). Check
//  * the Simulcast and SVC sections for more information.
//  *
//  * The RTP receive parameters describe a media stream as sent by mediasoup to
//  * an endpoint through its corresponding mediasoup Consumer. The mid value is
//  * unset (mediasoup does not include the MID RTP extension into RTP packets
//  * being sent to endpoints).
//  *
//  * There is a single entry in the encodings array (even if the corresponding
//  * producer uses simulcast). The consumer sends a single and continuous RTP
//  * stream to the endpoint and spatial/temporal layer selection is possible via
//  * consumer.setPreferredLayers().
//  *
//  * As an exception, previous bullet is not true when consuming a stream over a
//  * PipeTransport, in which all RTP streams from the associated producer are
//  * forwarded verbatim through the consumer.
//  *
//  * The RTP receive parameters will always have their ssrc values randomly
//  * generated for all of its  encodings (and optional rtx: { ssrc: XXXX } if the
//  * endpoint supports RTX), regardless of the original RTP send parameters in
//  * the associated producer. This applies even if the producer"s encodings have
//  * rid set.
//  */
// struct RtpParameters =
// {
// 	/**
// 	 * The MID RTP extension value as defined in the BUNDLE specification.
// 	 */
// 	std::string mid;
// 
// 	/**
// 	 * Media and RTX codecs in use.
// 	 */
// 	codecs: RtpCodecParameters[];
// 
// 	/**
// 	 * RTP header extensions in use.
// 	 */
// 	headerExtensions?: RtpHeaderExtensionParameters[];
// 
// 	/**
// 	 * Transmitted RTP streams and their settings.
// 	 */
// 	encodings?: RtpEncodingParameters[];
// 
// 	/**
// 	 * Parameters used for RTCP.
// 	 */
// 	rtcp?: RtcpParameters;
// }
// 
// /**
//  * Provides information on codec settings within the RTP parameters. The list
//  * of media codecs supported by mediasoup and their settings is defined in the
//  * supportedRtpCapabilities.ts file.
//  */
// struct RtpCodecParameters =
// {
// 	/**
// 	 * The codec MIME media type/subtype (e.g. "audio/opus", "video/VP8").
// 	 */
// 	std::string mimeType;
// 
// 	/**
// 	 * The value that goes in the RTP Payload Type Field. Must be unique.
// 	 */
// 	uint32_t payloadType;
// 
// 	/**
// 	 * Codec clock rate expressed in Hertz.
// 	 */
// 	uint32_t clockRate;
// 
// 	/**
// 	 * The uint32_t of channels supported (e.g. two for stereo). Just for audio.
// 	 * Default 1.
// 	 */
// 	uint32_t channels;
// 
// 	/**
// 	 * Codec-specific parameters available for signaling. Some parameters (such
// 	 * as "packetization-mode" and "profile-level-id" in H264 or "profile-id" in
// 	 * VP9) are critical for codec matching.
// 	 */
// 	json parameters;
// 
// 	/**
// 	 * Transport layer and codec-specific feedback messages for this codec.
// 	 */
// 	rtcpFeedback?: RtcpFeedback[];
// }
// 
// /**
//  * Provides information on RTCP feedback messages for a specific codec. Those
//  * messages can be transport layer feedback messages or codec-specific feedback
//  * messages. The list of RTCP feedbacks supported by mediasoup is defined in the
//  * supportedRtpCapabilities.ts file.
//  */
// struct RtcpFeedback =
// {
// 	/**
// 	 * RTCP feedback type.
// 	 */
// 	std::string type;
// 
// 	/**
// 	 * RTCP feedback parameter.
// 	 */
// 	std::string parameter;
// }
// 
// /**
//  * Provides information relating to an encoding, which represents a media RTP
//  * stream and its associated RTX stream (if any).
//  */
// struct RtpEncodingParameters =
// {
// 	/**
// 	 * The media SSRC.
// 	 */
// 	uint32_t ssrc;
// 
// 	/**
// 	 * The RID RTP extension value. Must be unique.
// 	 */
// 	std::string rid;
// 
// 	/**
// 	 * Codec payload type this encoding affects. If unset, first media codec is
// 	 * chosen.
// 	 */
// 	uint32_t codecPayloadType;
// 
// 	/**
// 	 * RTX stream information. It must contain a numeric ssrc field indicating
// 	 * the RTX SSRC.
// 	 */
// 	rtx?: { 	uint32_t ssrc };
// 
// 	/**
// 	 * It indicates whether discontinuous RTP transmission will be used. Useful
// 	 * for audio (if the codec supports it) and for video screen sharing (when
// 	 * static content is being transmitted, this option disables the RTP
// 	 * inactivity checks in mediasoup). Default false.
// 	 */
// 	bool dtx;
// 
// 	/**
// 	 * Number of spatial and temporal layers in the RTP stream (e.g. "L1T3").
// 	 * See webrtc-svc.
// 	 */
// 	std::string scalabilityMode;
// 
// 	/**
// 	 * Others.
// 	 */
// 	uint32_t scaleResolutionDownBy;
// 	uint32_t maxBitrate;
// };
// 
// /**
//  * Defines a RTP header extension within the RTP parameters. The list of RTP
//  * header extensions supported by mediasoup is defined in the
//  * supportedRtpCapabilities.ts file.
//  *
//  * mediasoup does not currently support encrypted RTP header extensions and no
//  * parameters are currently considered.
//  */
// struct RtpHeaderExtensionParameters =
// {
// 	/**
// 	 * The URI of the RTP header extension, as defined in RFC 5285.
// 	 */
// 	std::string uri;
// 
// 	/**
// 	 * The numeric identifier that goes in the RTP packet. Must be unique.
// 	 */
// 	uint32_t id;
// 
// 	/**
// 	 * If true, the value in the header is encrypted as per RFC 6904. Default false.
// 	 */
// 	bool encrypt;
// 
// 	/**
// 	 * Configuration parameters for the header extension.
// 	 */
// 	json parameters;
// }
// 
// /**
//  * Provides information on RTCP settings within the RTP parameters.
//  *
//  * If no cname is given in a producer"s RTP parameters, the mediasoup transport
//  * will choose a random one that will be used into RTCP SDES messages sent to
//  * all its associated consumers.
//  *
//  * mediasoup assumes reducedSize to always be true.
//  */
// struct RtcpParameters =
// {
// 	/**
// 	 * The Canonical Name (CNAME) used by RTCP (e.g. in SDES messages).
// 	 */
// 	std::string cname;
// 
// 	/**
// 	 * Whether reduced size RTCP RFC 5506 is configured (if true) or compound RTCP
// 	 * as specified in RFC 3550 (if false). Default true.
// 	 */
// 	bool reducedSize;
// 
// 	/**
// 	 * Whether RTCP-mux is used. Default true.
// 	 */
// 	bool mux;
// };

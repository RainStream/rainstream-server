#include "common.hpp"

#include "config.hpp"

json config =
R"(
{
	"domain" : "localhost",
	"https" :
	{
		"listenIp"   : "0.0.0.0",
		"listenPort" : 4443,
		"tls"        :
		{
			"cert" : "./certs/fullchain.pem",
			"key"  : "./certs/privkey.pem"
		}
	},
	"mediasoup" :
	{
		"numWorkers" : 1,
		"workerSettings" :
		{
			"logLevel" : "warn",
			"logTags"  :
			[
				"info",
				"ice",
				"dtls",
				"rtp",
				"srtp",
				"rtcp",
				"rtx",
				"bwe",
				"score",
				"simulcast",
				"svc",
				"sctp"
			],
			"rtcMinPort" : 40000,
			"rtcMaxPort" : 49999
		},
		"routerOptions" :
		{
			"mediaCodecs" :
			[
				{
					"kind"      : "audio",
					"mimeType"  : "audio/opus",
					"clockRate" : 48000,
					"channels"  : 2
				},
				{
					"kind"       : "video",
					"mimeType"   : "video/VP8",
					"clockRate"  : 90000,
					"parameters" :
					{
						"x-google-start-bitrate" : 1000
					}
				},
				{
					"kind"       : "video",
					"mimeType"   : "video/VP9",
					"clockRate"  : 90000,
					"parameters" :
					{
						"profile-id"             : 2,
						"x-google-start-bitrate" : 1000
					}
				}
			]
		},
		"webRtcTransportOptions" :
		{
			"listenIps" :
			[
				{
					"ip"          : "0.0.0.0",
					"announcedIp" : "192.168.0.100"
				}
			],
			"initialAvailableOutgoingBitrate" : 1000000,
			"minimumAvailableOutgoingBitrate" : 600000,
			"maxSctpMessageSize"              : 262144,
			"maxIncomingBitrate"              : 1500000
		},

		"plainTransportOptions" :
		{
			"listenIp" :
			{
				"ip"          : "0.0.0.0",
				"announcedIp" : "192.168.0.100"
			},
			"maxSctpMessageSize" : 262144
		}
	}
}
)"_json;

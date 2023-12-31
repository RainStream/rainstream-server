#include "common.h"

#include "config.hpp"
json config;
/*json config =
R"(
{
	"domain": "localhost",
	"https": {
		"listenIp": "0.0.0.0",
		"listenPort": 4443,
		"tls": {
			"cert": "certs/fullchain.pem",
			"key": "certs/privkey.pem",
			"phrase" : ""
		}
	},
	"mediasoup": {
		"numWorkers": 1,
		"workerSettings": {
			"logLevel": "warn",
			"logTags": [
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
			"rtcMinPort": 40000,
			"rtcMaxPort": 49999
		},
		"routerOptions": {
			"mediaCodecs": [
				{
					"kind": "audio",
					"mimeType": "audio/opus",
					"clockRate": 48000,
					"channels": 2
				},
				{
					"kind": "video",
					"mimeType": "video/VP8",
					"clockRate": 90000,
					"parameters": {
						"x-google-start-bitrate": 1000
					}
				},
				{
					"kind": "video",
					"mimeType": "video/VP9",
					"clockRate": 90000,
					"parameters": {
						"profile-id": 2,
						"x-google-start-bitrate": 1000
					}
				},
				{
					"kind": "video",
					"mimeType": "video/h264",
					"clockRate": 90000,
					"parameters": {
						"packetization-mode": 1,
						"profile-level-id": "4d0032",
						"level-asymmetry-allowed": 1,
						"x-google-start-bitrate": 1000
					}
				},
				{
					"kind": "video",
					"mimeType": "video/h264",
					"clockRate": 90000,
					"parameters": {
						"packetization-mode": 1,
						"profile-level-id": "42e01f",
						"level-asymmetry-allowed": 1,
						"x-google-start-bitrate": 1000
					}
				}
			]
		},
		"webRtcServerOptions": {
			"listenInfos": [
				{
					"protocol": "udp",
					"ip": "0.0.0.0",
					"announcedIp": "192.168.1.10",
					"port": 44444
				},
				{
					"protocol": "tcp",
					"ip": "0.0.0.0",
					"announcedIp": "192.168.1.10",
					"port": 44444
				}
			]
		},
		"webRtcTransportOptions": {
			"listenIps": [
				{
					"ip": "0.0.0.0",
					"announcedIp": "192.168.1.10"
				}
			],
			"initialAvailableOutgoingBitrate": 1000000,
			"minimumAvailableOutgoingBitrate": 600000,
			"maxSctpMessageSize": 262144,
			"maxIncomingBitrate": 1500000
		},
		"plainTransportOptions": {
			"listenIp": {
				"ip": "0.0.0.0",
				"announcedIp": "192.168.1.10"
			},
			"maxSctpMessageSize": 262144
		}
	}
}
)"_json;
*/


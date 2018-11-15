#define MS_CLASS "ClusterServer"

#include "ClusterServer.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "Room.hpp"
#include "RainStreamError.hpp"
#include <iostream>
#include <fstream>

#include "WebSocketServer.hpp"
#include "WebSocketClient.hpp"

const Json DefaultConfig = 

R"(
{
	"domain" : "127.0.0.1",
	"tls" :
	{
		"cert" : "certs/rainstream.localhost.cert.pem",
		"key" : "certs/rainstream.localhost.key.pem"
	},
	"rainstream" :
	{
		"logLevel" : "warn",
		"logTags" : ["info", "rtp", "rtcp", "rtx"],
		"rtcIPv4" : true,
		"rtcIPv6" : false,
		"rtcMaxPort" : 49999,
		"rtcMinPort" : 40000,
		"numWorkers" : 1,

	"mediaCodecs" :
	[
		{
			"kind"       : "audio",
			"name" : "opus",
			"clockRate" : 48000,
			"channels" : 2,
			"parameters" :
			{
				"useinbandfec" : 1
			}
		},
		{
			"kind"       : "video",
			"name" : "H264",
			"clockRate" : 90000,
			"parameters" :
			{
				"packetization-mode" : 1
			}
		}
	],
	"maxBitrate" : 500000
	}
})"_json;


/* Instance methods. */
ClusterServer::ClusterServer()
	: config(DefaultConfig)
{
	if (!Settings::configuration.configFile.empty())
	{
		std::ifstream in(Settings::configuration.configFile.c_str());
		if (in.is_open())
		{
			Json newConfig;
			in >> newConfig;
			in.close();

			config = Object::assign(config, newConfig);
		}
	}

	// rainstream server.
	Json rainstream = 
	{
		{ "numWorkers"       , 1 },
		{ "logLevel"         , config["rainstream"]["logLevel"] },
		{ "logTags"          , config["rainstream"]["logTags"] },
		{ "rtcIPv4"          , config["rainstream"]["rtcIPv4"] },
		{ "rtcIPv6"          , config["rainstream"]["rtcIPv6"] },
		{ "rtcAnnouncedIPv4" , config["rainstream"]["rtcAnnouncedIPv4"] },
		{ "rtcAnnouncedIPv6" , config["rainstream"]["rtcAnnouncedIPv6"] },
		{ "rtcMinPort"       , config["rainstream"]["rtcMinPort"] },
		{ "rtcMaxPort"       , config["rainstream"]["rtcMaxPort"] }
	};

	mediaServer = new rs::Server(rainstream,this);

	// HTTPS server for the protoo WebSocket server.
	Json tls =
	{
		{ "cert" , config["tls"]["cert"] },
		{ "key"  , config["tls"]["key"] }
	};

	webSocketServer = new protoo::WebSocketServer(tls, this);
	if (webSocketServer->Setup(config["domain"].get<std::string>().c_str(),
		Settings::configuration.serverPort))
	{
		LOG(INFO) << "WebSocket server running on port "
			<< Settings::configuration.serverPort;
	}

	LOG(INFO) << "ClusterServer Started";
}

ClusterServer::~ClusterServer()
{

}

void ClusterServer::OnServerClose()
{

}

void ClusterServer::OnNewRoom(rs::Room* room)
{

}

void ClusterServer::OnRoomClose(std::string roomId)
{
	LOG(INFO) << "Room has closed [roomId:" << roomId << "]";
	rooms_.erase(roomId);
}

void ClusterServer::OnConnectRequest(protoo::WebSocketClient* transport)
{
	std::string url = transport->url();

	std::string roomId = Utils::Url::Request(url, "roomId");
	std::string peerName = Utils::Url::Request(url, "peerName");

	if (roomId.empty() || peerName.empty())
	{
		LOG(ERROR) << "Connection request without roomId and/or peerName";

		transport->Close(400, "Connection request without roomId and/or peerName");

		return;
	}

	LOG(INFO) << "Peer[peerName:" << peerName << "] request join room [roomId:" << roomId << "]";

	Room* room = nullptr;

	// If an unknown roomId, create a new Room.
	if (!rooms_.count(roomId))
	{
		LOG(INFO) << "Creating a new room [roomId:" << roomId << "]";

		try
		{
			room = new Room(roomId, config["rainstream"]["mediaCodecs"], mediaServer, this);
		}
		catch (rs::Error error)
		{
			LOG(ERROR) << "error creating a new Room:" << error.ToString();

			reject(error);

			return;
		}

		rooms_[roomId] = room;
	}
	else
	{
		room = rooms_[roomId];
	}

	room->handleConnection(peerName, transport);
}

void ClusterServer::OnConnectClosed(protoo::WebSocketClient* transport)
{

}


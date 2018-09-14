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


/* Instance methods. */
ClusterServer::ClusterServer()
	: config(Json::object())
{
	if (!Settings::configuration.configFile.empty())
	{
		std::ifstream in(Settings::configuration.configFile.c_str());
		if (in.is_open())
		{
			in >> config;
			in.close();
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
		LOG(INFO) << "protoo WebSocket server running";
	}

	LOG(INFO) << "ClusterServer StartUp";
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
	rooms_.erase(roomId);
}

void ClusterServer::OnConnectRequest(protoo::WebSocketClient* transport)
{
	std::string url = transport->url();

	std::string roomId = Utils::Url::Request(url, "roomId");
	std::string peerName = Utils::Url::Request(url, "peerName");

	if (roomId.empty() || peerName.empty())
	{
		LOG(ERROR) << "connection request without roomId and/or peerName";

		transport->Close(400, "Connection request without roomId and/or peerName");

		return;
	}

	Room* room = nullptr;

	// If an unknown roomId, create a new Room.
	if (!rooms_.count(roomId))
	{
		LOG(INFO) << "creating a new Room [roomId:" << roomId << "]";

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


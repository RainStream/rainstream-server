#define MSC_CLASS "MediaServer"

#include "MediaServer.hpp"
#include <Logger.hpp>
#include <Worker.hpp>
#include "Utils.hpp"
#include "Room.hpp"
#include "Request.hpp"
#include "Settings.hpp"
#include "WebSocketServer.hpp"
#include "WebSocketClient.hpp"
#include <regex>
#include <iostream>
#include <fstream>
#include <uWS.h>

class Url
{
public:
	static std::string Request(const std::string& url, const std::string& request);
};

/* Inline static methods. */

inline std::string Url::Request(const std::string& url, const std::string& request)
{
	std::smatch result;
	if (std::regex_search(url.cbegin(), url.cend(), result, std::regex(request + "=(.*?)&"))) {
		// 匹配具有多个参数的url

		// *? 重复任意次，但尽可能少重复  
		return result[1];
	}
	else if (regex_search(url.cbegin(), url.cend(), result, std::regex(request + "=(.*)"))) {
		// 匹配只有一个参数的url

		return result[1];
	}
	else {
		// 不含参数或制定参数不存在

		return std::string();
	}
}

static int nextMediasoupWorkerIdx = 0;


/* Instance methods. */
MediaServer::MediaServer()
	: timer(nullptr)
{
	webSocketServer = new protoo::WebSocketServer(this);
	if (webSocketServer->Connect(Settings::configuration.serverUrl))
	{
		MSC_ERROR("connect to server: %s", Settings::configuration.serverUrl.c_str());
	}

	MSC_DEBUG("MediaServer Started");

	runMediasoupWorkers();
}

MediaServer::~MediaServer()
{

}

void MediaServer::OnRoomClose(std::string roomId)
{
	MSC_DEBUG("Room has closed [roomId:\"%s\"]", roomId.c_str());
	rooms_.erase(roomId);
}

void MediaServer::OnConnected(protoo::WebSocketClient* transport)
{
	transport->setListener(this);

	json data = {
		{ "nodeId", Settings::configuration.nodeId },
		{ "serviceType", "media_server" },
		{ "url", "" },
		{ "host", "" },
		{ "ip", "" },
		{ "port", 0 },
		{ "maxRoomCount", 1000 },
		{ "maxPeerCount", 4000 },
		{ "activeRoomCount", 0 },
		{ "activePeerCount", 0 },
		{ "status", 1 }
	};
	transport->Register("registerNode", data);

	/*
	2.reportNodeOnline
	周期性(如每分2分钟)汇报节点(包括gateway/signal_server类型)的在线信息
	request:
	{
	"method": 'reportNodeOnline',
		"data": {
			"nodeId": "xxxxx",
			"activeRoomCount": 100,
			"activePeerCount": 0,
			"status": 1
		}
	}
	 */
}

std::future<void> MediaServer::OnRequest(protoo::WebSocketClient* transport, protoo::Request* request)
{
	MSC_DEBUG("Peer[peerId:%s] request join room [roomId:%s]",
		request->peerId.c_str(), request->roomId.c_str());

	Room* room = co_await getOrCreateRoom(request->roomId);
	co_await room->handleProtooRequest(transport, request);

	co_return;
}

void MediaServer::OnClosed(int code, const std::string& message)
{

}

std::future<void> MediaServer::connectionrequest(protoo::WebSocketClient* transport)
{
// 	std::string url = transport->url();
// 
// 	std::string roomId = Url::Request(url, "roomId");
// 	std::string peerId = Url::Request(url, "peerId");
// 
// 	if (roomId.empty() || peerId.empty())
// 	{
// 		MSC_ERROR("Connection request without roomId and/or peerName");
// 
// 		transport->Close(400, "Connection request without roomId and/or peerName");
// 
// 		return;
// 	}
// 
// 	MSC_DEBUG("Peer[peerId:%s] request join room [roomId:%s]",
// 		peerId.c_str(), roomId.c_str());
// 
// 	Room* room = co_await getOrCreateRoom(roomId);
// 	room->handleConnection(peerId, true, transport);
// 
// 	co_return;
}

void MediaServer::runMediasoupWorkers()
{
	int numWorkers = 1;

	MSC_DEBUG("running %d mediasoup Workers...", numWorkers);

	json config;
	std::ifstream in(Settings::configuration.configFile.c_str());
	in >> config;

	json settings = config["mediasoup"]["workerSettings"];

	for (int i = 0; i < numWorkers; ++i)
	{
		Worker* worker = new Worker(settings);

		worker->on("died", [=]()
		{
			MSC_ERROR("mediasoup Worker died, exiting  in 2 seconds... [pid:%d]", worker->pid());

			//setTimeout(() = > process.exit(1), 2000);
		});

		mediasoupWorkers.push_back(worker);

		// Log worker resource usage every X seconds.
// 		setInterval(async() = >
// 		{
// 			const usage = await worker.getResourceUsage();
// 
// 			logger.info('mediasoup Worker resource usage [pid:%d]: %o', worker.pid, usage);
// 		}, 120000);
	}
}

Worker* MediaServer::getMediasoupWorker()
{
	Worker* worker = mediasoupWorkers[nextMediasoupWorkerIdx];

	if (++nextMediasoupWorkerIdx == mediasoupWorkers.size())
		nextMediasoupWorkerIdx = 0;

	return worker;
}

std::future<Room*> MediaServer::getOrCreateRoom(std::string roomId)
{
	Room* room;

	// If the Room does not exist create a new one.
	if (!rooms_.count(roomId))
	{
		MSC_DEBUG("creating a new Room [roomId:%s]", roomId.c_str());

		Worker* mediasoupWorker = getMediasoupWorker();

		room = co_await Room::create(mediasoupWorker, roomId);

		rooms_.insert(std::make_pair(roomId, room));
		room->on("close", [=]() { rooms_.erase(roomId); });
	}
	else
	{
		room = rooms_.at(roomId);
	}

	co_return room;
}

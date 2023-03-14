#define MSC_CLASS "ClusterServer"

#include "ClusterServer.hpp"
#include <Logger.h>
#include <Worker.h>
#include <WebRtcServer.h>
#include "Utils.h"
#include "Room.hpp"
#include "WebSocketServer.hpp"
#include "WebSocketClient.hpp"
#include "Utility.hpp"
#include "config.hpp"

#define MEDIASOUP_USE_WEBRTC_SERVER true

static int nextMediasoupWorkerIdx = 0;


/* Instance methods. */
ClusterServer::ClusterServer()
{
	loadGlobalCheck();

// 	if (!Settings::configuration.configFile.empty())
// 	{
// 		std::ifstream in(Settings::configuration.configFile.c_str());
// 		if (in.is_open())
// 		{
// 			json newConfig;
// 			in >> newConfig;
// 			in.close();
// 
// 			config = Object::assign(config, newConfig);
// 		}
// 	}

	json tmp_config;
	std::ifstream in("config.json");
	if (in.is_open())
	{
		in >> tmp_config;
		in.close();
	}
	
	if (tmp_config.is_null() || !tmp_config.is_object())
	{
		MSC_ABORT("config.json is not exist or empty!");
	}

	config = tmp_config;

	// HTTPS server for the protoo WebSocket server.
	json tls = config["https"]["tls"];
	std::string listenIp = config["https"]["listenIp"];
	uint16_t listenPort = config["https"]["listenPort"];

	_webSocketServer = new protoo::WebSocketServer(tls, this);
	if (_webSocketServer->Setup(listenIp.c_str(), listenPort))
	{
		MSC_DEBUG("WebSocket server running on port: %d", listenPort);
	}

	MSC_DEBUG("ClusterServer Started");

	runMediasoupWorkers().start([](auto&&) {});

	//runHttpsServer();

	// Log rooms status every X seconds.
	setInterval([&]()
	{
		for (auto &[key, room] : this->_rooms)
		{
			room->logStatus();
		}
	}, 120000);
}

ClusterServer::~ClusterServer()
{
	for (Worker* worker : _mediasoupWorkers)
	{
		worker->close();

		delete worker;
	}
}

void ClusterServer::OnConnectRequest(std::string requestUrl, const protoo::FnAccept& accept, const  protoo::FnReject& reject)
{
	std::string roomId = Url::Request(requestUrl, "roomId");
	std::string peerId = Url::Request(requestUrl, "peerId");

	if (roomId.empty() || peerId.empty())
	{
		MSC_ERROR("Connection request without roomId and/or peerName");

		reject(Error("Connection request without roomId and/or peerName"));

		return;
	}

	MSC_DEBUG("Peer[peerId:%s] request join room [roomId:%s]",
		peerId.c_str(), roomId.c_str());

	this->_queue.push([=]() -> async_simple::coro::Lazy<void>
	{
		Room* room = co_await getOrCreateRoom(roomId);

		MSC_WARN("Peer[peerId:%s] handleConnection [roomId:%s]",
			peerId.c_str(), roomId.c_str());

		auto transport = accept();

		room->handleConnection(peerId, true, transport);
	});
}


void ClusterServer::OnConnectClosed(protoo::WebSocketClient* transport)
{

}

async_simple::coro::Lazy<void> ClusterServer::runMediasoupWorkers()
{
	uv_cpu_info_t* info;
	int cpu_count;
	uv_cpu_info(&info, &cpu_count);
	uv_free_cpu_info(info, cpu_count);

	int numWorkers = config["mediasoup"].value("numWorkers", cpu_count);
	bool singleProcess = config["mediasoup"].value("singleProcess", false);
	bool useWebrtcServer = config["mediasoup"].value("useWebrtcServer", MEDIASOUP_USE_WEBRTC_SERVER);

	if (numWorkers <= 0)
	{
		
		
		numWorkers = cpu_count;
	}

	MSC_DEBUG("running %d mediasoup Workers...", numWorkers);

	for (int i = 0; i < numWorkers; ++i)
	{
		const json& workerSettings = config["mediasoup"]["workerSettings"];
		json settings = {
			{ "logLevel", workerSettings["logLevel"]},
			{ "logTags", workerSettings["logTags"] },
			{ "rtcMinPort", workerSettings["rtcMinPort"] },
			{ "rtcMaxPort", workerSettings["rtcMaxPort"] }
		};

		Worker* worker = Worker::Create(settings, singleProcess);

		worker->on("died", [=]()
		{
			MSC_ERROR("mediasoup Worker died, exiting  in 2 seconds... [pid:%d]", worker->pid());

			setTimeout([=]() { std::_Exit(EXIT_FAILURE); } ,2000);
		});

		_mediasoupWorkers.push_back(worker);

		// Create a WebRtcServer in this Worker.
		if (useWebrtcServer != false)
		{
			// Each mediasoup Worker will run its own WebRtcServer, so those cannot
			// share the same listening ports. Hence we increase the value in config.js
			// for each Worker.

			WebRtcServerOptions webRtcServerOptions = config["mediasoup"]["webRtcServerOptions"];
			int portIncrement = _mediasoupWorkers.size() - 1;

			for (auto& listenInfo : webRtcServerOptions.listenInfos)
			{
				listenInfo.port += portIncrement;
			}

			WebRtcServer* webRtcServer = co_await worker->createWebRtcServer(webRtcServerOptions);

			this->_workerWebRtcServers.insert(std::pair(worker, webRtcServer));
		}

		// Log worker resource usage every X seconds.
		auto getResourceUsage = 
 		setInterval([=]()->async_simple::coro::Lazy<void>
 		{
 			json usage = co_await worker->getResourceUsage();
 
 			MSC_DEBUG("mediasoup Worker resource usage[pid:%d]: %s", worker->pid(), usage.dump().c_str());

			co_return;
 		}, 120000);
	}
}

async_simple::coro::Lazy<void> ClusterServer::runHttpsServer()
{
	_webSocketServer->get("/rooms/:roomId", [=](auto* res, auto* req) {
		std::string roomId(req->getParameter(0));
		if (!this->_rooms.contains(roomId))
		{
			res->writeStatus("500")->end("can not find room");
			return;
		}
		
		Room* room = this->_rooms[roomId];
		json data = room->getRouterRtpCapabilities();

		res->end(data.dump());
	});

	_webSocketServer->post("/rooms/:roomId/broadcasters", [=](auto* res, auto* req) {
		std::string buffer;
		res->onData([=, buffer = std::move(buffer)](std::string_view data, bool last) mutable
			{
				buffer.append(data.data(), data.length());

				if (last)
				{
					try
					{
						json body = json::parse(buffer);
						std::string id = body["id"];
						std::string displayName = body["displayName"];
						std::string device = body["device"];
						json rtpCapabilities = body["rtpCapabilities"];

						std::string roomId(req->getParameter(0));
						if (!this->_rooms.contains(roomId))
						{
							MSC_THROW_ERROR("can not find room");
						}

						Room* room = this->_rooms[roomId];

						/*room->createBroadcaster(id, displayName, device, rtpCapabilities)
							.then([=](json response) 
								{
									res->end(response.dump());
								});*/
					}
					catch (const std::exception& e)
					{
						res->writeStatus("500")->end(e.what());
					}
				}
			});

		res->onAborted([=]() {
			MSC_ERROR("onAborted");
			});
	});

	_webSocketServer->del("/rooms/:roomId/broadcasters/:broadcasterId", [=](auto* res, auto* req) {
		std::string roomId(req->getParameter(0));
		std::string broadcasterId(req->getParameter(1));

		if (!this->_rooms.contains(roomId))
		{
			res->writeStatus("500")->end("can not find room");
			return;
		}

		Room* room = this->_rooms[roomId];
		room->deleteBroadcaster(broadcasterId);

		res->end("broadcaster deleted");
	});

	_webSocketServer->post("/rooms/:roomId/broadcasters/:broadcasterId/transports", [](auto* res, auto* req) {
		std::string roomId(req->getParameter(0));
		std::string broadcasterId(req->getParameter(1));
		res->end("/rooms/:roomId/broadcasters/:broadcasterId/transports: " + roomId + " " + broadcasterId);
	});

	_webSocketServer->post("/rooms/:roomId/broadcasters/:broadcasterId/transports/:transportId/connect", [](auto* res, auto* req) {
		std::string roomId(req->getParameter(0));
		std::string broadcasterId(req->getParameter(1));
		std::string transportId(req->getParameter(2));
		res->end("/rooms/:roomId/broadcasters/:broadcasterId/transports: " + roomId + " " + broadcasterId);
	});

	_webSocketServer->post("/rooms/:roomId/broadcasters/:broadcasterId/transports/:transportId/producers", [](auto* res, auto* req) {
		std::string roomId(req->getParameter(0));
		std::string broadcasterId(req->getParameter(1));
		std::string transportId(req->getParameter(2));
		res->end("/rooms/:roomId/broadcasters/:broadcasterId/transports: " + roomId + " " + broadcasterId);
	});

	_webSocketServer->post("/rooms/:roomId/broadcasters/:broadcasterId/transports/:transportId/consume", [](auto* res, auto* req) {
		std::string roomId(req->getParameter(0));
		std::string broadcasterId(req->getParameter(1));
		std::string transportId(req->getParameter(2));
		res->end("/rooms/:roomId/broadcasters/:broadcasterId/transports: " + roomId + " " + broadcasterId);
	});

	_webSocketServer->post("/rooms/:roomId/broadcasters/:broadcasterId/transports/:transportId/consume/data", [](auto* res, auto* req) {
		std::string roomId(req->getParameter(0));
		std::string broadcasterId(req->getParameter(1));
		std::string transportId(req->getParameter(2));
		res->end("/rooms/:roomId/broadcasters/:broadcasterId/transports: " + roomId + " " + broadcasterId);
	});

	_webSocketServer->post("/rooms/:roomId/broadcasters/:broadcasterId/transports/:transportId/produce/data", [](auto* res, auto* req) {
		std::string roomId(req->getParameter(0));
		std::string broadcasterId(req->getParameter(1));
		std::string transportId(req->getParameter(2));
		res->end("/rooms/:roomId/broadcasters/:broadcasterId/transports: " + roomId + " " + broadcasterId);
	});

	co_return;
}

Worker* ClusterServer::getMediasoupWorker()
{
	Worker* worker = _mediasoupWorkers[nextMediasoupWorkerIdx];

	if (++nextMediasoupWorkerIdx == _mediasoupWorkers.size())
		nextMediasoupWorkerIdx = 0;

	return worker;
}

async_simple::coro::Lazy<Room*> ClusterServer::getOrCreateRoom(std::string roomId)
{
	Room* room;

	// If the Room does not exist create a new one.
	if (!_rooms.count(roomId))
	{
		MSC_DEBUG("creating a new Room [roomId:%s]", roomId.c_str());

		Worker* mediasoupWorker = getMediasoupWorker();

		WebRtcServer* webRtcServer = _workerWebRtcServers[mediasoupWorker];

		room = co_await Room::create(mediasoupWorker, roomId, webRtcServer);

		_rooms.insert(std::make_pair(roomId, room));
		room->on("close", [=]() { _rooms.erase(roomId); });
	}
	else
	{
		room = _rooms.at(roomId);
	}

	co_return room;
}

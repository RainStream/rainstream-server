#define MSC_CLASS "ClusterServer"

#include "ClusterServer.hpp"
#include <Logger.hpp>
#include <Worker.hpp>
#include <WebRtcServer.hpp>
#include "Utils.hpp"
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

	// mediasoup server.
	json mediasoup = 
	{
		{ "numWorkers"       , 1 },
		{ "logLevel"         , config["mediasoup"]["logLevel"] },
		{ "logTags"          , config["mediasoup"]["logTags"] },
		{ "rtcIPv4"          , config["mediasoup"]["rtcIPv4"] },
		{ "rtcIPv6"          , config["mediasoup"]["rtcIPv6"] },
		{ "rtcAnnouncedIPv4" , config["mediasoup"]["rtcAnnouncedIPv4"] },
		{ "rtcAnnouncedIPv6" , config["mediasoup"]["rtcAnnouncedIPv6"] },
		{ "rtcMinPort"       , config["mediasoup"]["rtcMinPort"] },
		{ "rtcMaxPort"       , config["mediasoup"]["rtcMaxPort"] }
	};

	// HTTPS server for the protoo WebSocket server.
	json tls = config["https"]["tls"];

	_webSocketServer = new protoo::WebSocketServer(tls, this);
	if (_webSocketServer->Setup("127.0.0.1", 4443))
	{
		MSC_DEBUG("WebSocket server running on port: %d", 4443);
	}

	MSC_DEBUG("ClusterServer Started");

	runMediasoupWorkers();

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

	this->_queue.push([=]()->task_t<void>
	{	
		Room* room = co_await getOrCreateRoom(roomId);

		auto transport = accept();

		room->handleConnection(peerId, true, transport);
	});
}


void ClusterServer::OnConnectClosed(protoo::WebSocketClient* transport)
{

}

task_t<void> ClusterServer::runMediasoupWorkers()
{
	int numWorkers = 1;

	MSC_DEBUG("running %d mediasoup Workers...", numWorkers);

	for (int i = 0; i < numWorkers; ++i)
	{
		json settings = {
			{ "logLevel", "warn" },
			{ "logTags", { "info", "ice", "dtls","rtp","srtp","rtcp", "rtx","bwe",	"score", "simulcast","svc",	"sctp" } },
			{ "rtcMinPort", 40000 },
			{ "rtcMaxPort", 49999 }
		};

		Worker* worker = new Worker(settings);

		worker->on("died", [=]()
		{
			MSC_ERROR("mediasoup Worker died, exiting  in 2 seconds... [pid:%d]", worker->pid());

			setTimeout([=]() { std::_Exit(EXIT_FAILURE); } ,2000);
		});

		_mediasoupWorkers.push_back(worker);

		// Create a WebRtcServer in this Worker.
		if (MEDIASOUP_USE_WEBRTC_SERVER != false)
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
 		setInterval([=]()->task_t<void>
 		{
 			json usage = co_await worker->getResourceUsage();
 
 			MSC_DEBUG("mediasoup Worker resource usage[pid:%d]: %s", worker->pid(), usage.dump().c_str());

			co_return;
 		}, 120000);
	}
}

Worker* ClusterServer::getMediasoupWorker()
{
	Worker* worker = _mediasoupWorkers[nextMediasoupWorkerIdx];

	if (++nextMediasoupWorkerIdx == _mediasoupWorkers.size())
		nextMediasoupWorkerIdx = 0;

	return worker;
}

task_t<Room*> ClusterServer::getOrCreateRoom(std::string roomId)
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

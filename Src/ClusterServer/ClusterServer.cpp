#define MSC_CLASS "ClusterServer"

#include "ClusterServer.hpp"
#include <Logger.hpp>
#include <Worker.hpp>
#include "Utils.hpp"
#include "Room.hpp"
#include "WebSocketServer.hpp"
#include "WebSocketClient.hpp"
#include <regex>

int gIndex = 0;

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

const json DefaultConfig = 

R"(
{
	"domain" : "127.0.0.1",
	"tls" :
	{
		"cert" : "certs/fullchain.pem",
		"key" : "certs/privkey.pem"
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

	// rainstream server.
	json rainstream = 
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

	// HTTPS server for the protoo WebSocket server.
	json tls =
	{
		{ "cert" , config["tls"]["cert"] },
		{ "key"  , config["tls"]["key"] }
	};

	_webSocketServer = new protoo::WebSocketServer(tls, this);
	if (_webSocketServer->Setup("127.0.0.1", 4443))
	{
		MSC_ERROR("WebSocket server running on port: %d", 4443);
	}

	MSC_DEBUG("ClusterServer Started");

	runMediasoupWorkers();
}

ClusterServer::~ClusterServer()
{

}

void ClusterServer::OnRoomClose(std::string roomId)
{
	MSC_DEBUG("Room has closed [roomId:\"%s\"]", roomId.c_str());
	_rooms.erase(roomId);
}

void ClusterServer::OnConnectRequest(protoo::WebSocketClient* transport)
{
	connectionrequest(transport);
}

void ClusterServer::connectionrequest(protoo::WebSocketClient* transport)
{
	std::string url = transport->url();

	std::string roomId = Url::Request(url, "roomId");
	std::string peerId = Url::Request(url, "peerId");

	if (roomId.empty() || peerId.empty())
	{
		MSC_ERROR("Connection request without roomId and/or peerName");

		transport->Close(400, "Connection request without roomId and/or peerName");

		return;
	}

	MSC_DEBUG("Peer[peerId:%s] request join room [roomId:%s]",
		peerId.c_str(), roomId.c_str());

	this->_queue.push([=]()->std::future<void>
		{
			Room* room = co_await getOrCreateRoom(roomId);

			MSC_DEBUG("get sync room %s for peer %s %d", roomId.c_str(), peerId.c_str(), ++gIndex);

			room->handleConnection(peerId, true, transport);
		});
}

void ClusterServer::OnConnectClosed(protoo::WebSocketClient* transport)
{

}

void ClusterServer::runMediasoupWorkers()
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

			//setTimeout(() = > process.exit(1), 2000);
		});

		_mediasoupWorkers.push_back(worker);

		// Log worker resource usage every X seconds.
// 		setInterval(async() = >
// 		{
// 			const usage = await worker.getResourceUsage();
// 
// 			logger.info('mediasoup Worker resource usage [pid:%d]: %o', worker.pid, usage);
// 		}, 120000);
	}
}

Worker* ClusterServer::getMediasoupWorker()
{
	Worker* worker = _mediasoupWorkers[nextMediasoupWorkerIdx];

	if (++nextMediasoupWorkerIdx == _mediasoupWorkers.size())
		nextMediasoupWorkerIdx = 0;

	return worker;
}

std::future<Room*> ClusterServer::getOrCreateRoom(std::string roomId)
{
	Room* room;

	// If the Room does not exist create a new one.
	if (!_rooms.count(roomId))
	{
		MSC_DEBUG("creating a new Room [roomId:%s]", roomId.c_str());

		Worker* mediasoupWorker = getMediasoupWorker();

		room = co_await Room::create(mediasoupWorker, roomId);

		_rooms.insert(std::make_pair(roomId, room));
		room->on("close", [=]() { _rooms.erase(roomId); });
	}
	else
	{
		room = _rooms.at(roomId);
	}

	co_return room;
}

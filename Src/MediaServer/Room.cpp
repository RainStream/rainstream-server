#define MSC_CLASS "Room"

#include "Room.hpp"
#include "config.hpp"
#include "Peer.hpp"
#include "Request.hpp"
#include <Logger.hpp>
#include <errors.hpp>
#include <Worker.hpp>
#include <Router.hpp>
#include <Transport.hpp>
#include <Producer.hpp>
#include <Consumer.hpp>
#include <WebRtcTransport.hpp>
#include "Utils.hpp"
#include "WebSocketClient.hpp"
#include <math.h>

const uint32_t MAX_BITRATE = 3000000;
const uint32_t MIN_BITRATE = 50000;
const float BITRATE_FACTOR = 0.75;


std::future<Room*> Room::create(Worker* mediasoupWorker, std::string roomId)
{
	MSC_DEBUG("create() [roomId:%s]", roomId.c_str());

	// Create a protoo Room instance.
	//const protooRoom = new protoo.Room();

	// Router media codecs.
	json mediaCodecs = config["mediasoup"]["routerOptions"]["mediaCodecs"];

	// Create a mediasoup Router.
	Router* mediasoupRouter = co_await mediasoupWorker->createRouter(mediaCodecs);

	// Create a mediasoup AudioLevelObserver.
// 	const audioLevelObserver = co_await mediasoupRouter.createAudioLevelObserver(
// 		{
// 			maxEntries: 1,
// 			threshold : -80,
// 			interval : 800
// 		});
// 
// 	const bot = co_await Bot.create({ mediasoupRouter });

	return new Room(
		roomId,
		mediasoupRouter);
}

Room::Room(std::string roomId, Router* router)
	: _roomId(roomId)
	, _mediasoupRouter(router)
{

}

Room::~Room()
{

}

std::string Room::id()
{
	return _roomId;
}

void Room::close()
{
	//DLOG(INFO) << "close()");

	this->_closed = true;

	// Close the mediasoup Room.
// 	if (this->_mediaRoom)
// 		this->_mediaRoom->close();

	// Emit "close" event.
	this->listener->OnRoomClose(_roomId);
}

std::future<void> Room::handleProtooRequest(protoo::WebSocketClient* transport, protoo::Request* request)
{
	std::string peerId = request->peerId;
	std::string method = request->method;

	json& data = request->data;

	if (method == "getRouterRtpCapabilities")
	{
		if (this->_peers.count(peerId))
		{
			protoo::Peer* existingPeer = this->_peers.at(peerId);

			MSC_WARN(
				"handleProtooConnection() | there is already a protoo Peer with same peerId, closing it [peerId:%s]",
				peerId.c_str());

			existingPeer->close();
		}

		protoo::Peer* peer{ nullptr };

		// Create a new protoo Peer with the given peerId.
		try
		{
			peer = new protoo::Peer(peerId, this->_roomId ,transport, this);
			this->_peers.insert(std::make_pair(peerId, peer));
		}
		catch (std::exception& error)
		{
			MSC_ERROR("protooRoom.createPeer() failed:%s", error.what());
		}

		// Use the peer->data object to store mediasoup related objects.

		// Not joined after a custom protoo "join" request is later received.
		peer->data.consume = true;
		peer->data.joined = false;


		request->Accept(this->_mediasoupRouter->rtpCapabilities());
	}
	else
	{
		try
		{
			protoo::Peer* peer = this->_peers.at(peerId);

			co_await _handleProtooRequest(peer, request);
		}
		catch (const std::exception&)
		{

		}
	}

	

// 	peer->on("close", [=]()
// 	{
// 		if (this->_closed)
// 			return;
// 
// 		MSC_DEBUG("protoo Peer \"close\" event [peerId:%s]", peer->id().c_str());
// 
// 		// If the Peer was joined, notify all Peers.
// 		if (peer->data.joined)
// 		{
// 			for (protoo::Peer* otherPeer : this->_getJoinedPeers(peer))
// 			{
// 				try
// 				{
// 					otherPeer->notify("peerClosed", json{ { "peerId", peer->id()} });
// 				}
// 				catch (const std::exception&)
// 				{
// 
// 				}
// 			}
// 		}
// 
// 		// Iterate and close all mediasoup Transport associated to this Peer, so all
// 		// its Producers and Consumers will also be closed.
// 		for (auto[key, transport] : peer->data.transports)
// 		{
// 			transport->close();
// 		}
// 
// 		// If this is the latest Peer in the room, close the room.
// 		if (this->_peers.size()== 0)
// 		{
// 			MSC_DEBUG(
// 				"last Peer in the room left, closing the room [roomId:%s]",
// 				this->_roomId.c_str());
// 
// 			this->close();
// 		}
// 	});
}

/**
* Handle protoo requests from browsers.
*
* @async
*/
std::future<void> Room::_handleProtooRequest(protoo::Peer* peer, protoo::Request* request)
{
	std::string method = request->method;
	json& data = request->data;

	if (method == "getRouterRtpCapabilities")
	{
		request->Accept(this->_mediasoupRouter->rtpCapabilities());
	}
	else if (method == "join")
	{
		// Ensure the Peer is not already joined.
		if (peer->data.joined)
			MSC_THROW_ERROR("Peer already joined");

		std::string displayName = data["displayName"];
		json device = data["device"];
		json rtpCapabilities = data["rtpCapabilities"];
		json sctpCapabilities = data["sctpCapabilities"];

		// Store client data into the protoo Peer data object.
		peer->data.joined = true;
		peer->data.displayName = displayName;
		peer->data.device = device;
		peer->data.rtpCapabilities = rtpCapabilities;
		peer->data.sctpCapabilities = sctpCapabilities;

		// Tell the new Peer about already joined Peers.
		// And also create Consumers for existing Producers.

		std::list<protoo::Peer*> joinedPeers = this->_getJoinedPeers();

		// Reply now the request with the list of joined peers (all but the new one).
		json peerInfos = json::array();
		for (protoo::Peer* joinedPeer : joinedPeers)
		{
			if (joinedPeer->id() != peer->id())
			{
				json info = {
					{ "id", joinedPeer->id() } ,
					{ "displayName", joinedPeer->data.displayName },
					{ "device", joinedPeer->data.device }
				};

				peerInfos.push_back(info);
			}
		}

		request->Ignore();

		// Mark the new Peer as joined.
		peer->data.joined = true;

		for (protoo::Peer* joinedPeer : joinedPeers)
		{
			// Create Consumers for existing Producers.
			for (auto[key, producer] : joinedPeer->data.producers)
			{
				this->_createConsumer(peer, joinedPeer, producer);
			}

			/*
			// Create DataConsumers for existing DataProducers.
			for (const dataProducer of joinedPeer->data.dataProducers.values())
			{
				if (dataProducer.label == "bot")
					continue;

				this->_createDataConsumer(
					{
						dataConsumerPeer: peer,
						dataProducerPeer : joinedPeer,
						dataProducer
					});
			}
			*/
		}

		/*
		// Create DataConsumers for bot DataProducer.
		this->_createDataConsumer(
			{
				dataConsumerPeer: peer,
				dataProducerPeer : null,
				dataProducer : this->_bot.dataProducer
			});
		*/
		// Notify the new Peer to all other Peers.
// 		for (protoo::Peer* otherPeer : this->_getJoinedPeers(peer))
// 		{
// 			try
// 			{
// 				otherPeer->notify(
// 					"newPeer",
// 					json{
// 						{ "id", peer->id() },
// 						{ "displayName", peer->data.displayName },
// 						{ "device", peer->data.device }
// 					});
// 			}
// 			catch (const std::exception&)
// 			{
// 
// 			}
// 				
// 		}
	}
	else if (method == "createWebRtcTransport")
	{
		// NOTE: Don"t require that the Peer is joined here, so the client can
		// initiate mediasoup Transports and be ready when he later joins.

		bool forceTcp = data["forceTcp"];
		bool producing = data["producing"];
		bool consuming = data["consuming"];
		json sctpCapabilities = data["sctpCapabilities"];

		json webRtcTransportOptions = config["mediasoup"]["webRtcTransportOptions"];

		WebRtcTransportOptions options;
		options.listenIps = webRtcTransportOptions["listenIps"];
		options.initialAvailableOutgoingBitrate = webRtcTransportOptions["initialAvailableOutgoingBitrate"];
		options.maxSctpMessageSize = webRtcTransportOptions["maxSctpMessageSize"];
		options.enableSctp = sctpCapabilities.is_object();
		options.numSctpStreams = sctpCapabilities.value("numStreams", json::object());
		options.appData = { { "producing", producing }, { "consuming", consuming } };

		if (forceTcp)
		{
			options.enableUdp = false;
			options.enableUdp = true;
		}

		WebRtcTransport* transport = co_await this->_mediasoupRouter->createWebRtcTransport(
			options);

		transport->on("sctpstatechange", [=](std::string sctpState)
		{
			MSC_DEBUG("WebRtcTransport \"sctpstatechange\" event [sctpState:%s]", sctpState.c_str());
		});

		transport->on("dtlsstatechange", [=](std::string dtlsState)
		{
			if (dtlsState == "failed" || dtlsState == "closed")
				MSC_WARN("WebRtcTransport \"dtlsstatechange\" event [dtlsState:%s]", dtlsState.c_str());
		});

		// NOTE: For testing.
		// co_await transport->enableTraceEvent([ "probation", "bwe" ]);
		co_await transport->enableTraceEvent({ "bwe" });

		transport->on("trace", [=](json &trace)
		{
			MSC_DEBUG(
				"transport \"trace\" event [transportId:%s, trace.type:%s, trace:%s]",
				transport->id().c_str(), trace["type"].get<std::string>().c_str(), trace.dump().c_str());

			if (trace["type"] == "bwe" && trace["direction"] == "out")
			{
				try
				{
					peer->notify(
						"downlinkBwe",
						json{
							{ "desiredBitrate", trace["info"]["desiredBitrate"] },
							{ "effectiveDesiredBitrate", trace["info"]["effectiveDesiredBitrate"] },
							{ "availableBitrate", trace["info"]["availableBitrate"] }
						});
				}
				catch (const std::exception&)
				{

				}
			}
		});

		// Store the WebRtcTransport into the protoo Peer data Object.
		peer->data.transports.insert(std::make_pair(transport->id(), transport));

		request->Accept(
			json{
				{ "id", transport->id() },
				{ "iceParameters", transport->iceParameters() },
				{ "iceCandidates", transport->iceCandidates() },
				{ "dtlsParameters", transport->dtlsParameters() },
				{ "sctpParameters", transport->sctpParameters() }
			});

		uint32_t maxIncomingBitrate = config["mediasoup"]["webRtcTransportOptions"]["maxIncomingBitrate"];

		// If set, apply max incoming bitrate limit.
		if (maxIncomingBitrate)
		{
			try {
				co_await transport->setMaxIncomingBitrate(maxIncomingBitrate);
			}
			catch (std::exception& error)
			{
			}
		}
	}
	else if (method == "connectWebRtcTransport")
	{
		std::string transportId = data["transportId"];
		json dtlsParameters = data["dtlsParameters"];

		if (!peer->data.transports.count(transportId))
			MSC_THROW_ERROR("transport with id \"%s\" not found", transportId.c_str());

		Transport* transport = peer->data.transports.at(transportId);

		co_await transport->connect(dtlsParameters);

		request->Accept(json::object());
	}
	else if (method == "restartIce")
	{
		std::string transportId = data["transportId"];

		if (!peer->data.transports.count(transportId))
			MSC_THROW_ERROR("transport with id \"%s\" not found", transportId.c_str());

		WebRtcTransport* transport = dynamic_cast<WebRtcTransport*>(peer->data.transports.at(transportId));

		json iceParameters = co_await transport->restartIce();

		request->Accept(iceParameters);
	}
	else if (method == "produce")
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		std::string transportId = data["transportId"];
		std::string kind = data["kind"];
		json rtpParameters = data["rtpParameters"];
		json appData = data["appData"];

		if (!peer->data.transports.count(transportId))
			MSC_THROW_ERROR("transport with id \"%s\" not found", transportId.c_str());

		Transport* transport = peer->data.transports.at(transportId);


		// Add peerId into appData to later get the associated Peer during
		// the "loudest" event of the audioLevelObserver.
		appData["peerId"] = peer->id();

		Producer* producer = co_await transport->produce(std::string(), kind,
				rtpParameters, false, 0, appData);

		// Store the Producer into the protoo Peer data Object.
		peer->data.producers.insert(std::make_pair(producer->id(), producer));

		// Set Producer events.
		producer->on("score", [=](json score)
		{
			// MSC_DEBUG(
			// 	"producer "score" event [producerId:%s, score:%o]",
			// 	producer->id, score);

			try
			{
				peer->notify("producerScore", json{ { "producerId", producer->id() }, { "score", score } });
			}
			catch (const std::exception&)
			{

			}
		});

		producer->on("videoorientationchange", [=](json videoOrientation)
		{
			MSC_DEBUG(
				"producer \"videoorientationchange\" event [producerId:%s, videoOrientation:%s]",
				producer->id().c_str(), videoOrientation.dump().c_str());
		});

		// NOTE: For testing.
		// co_await producer->enableTraceEvent([ "rtp", "keyframe", "nack", "pli", "fir" ]);
		// co_await producer->enableTraceEvent([ "pli", "fir" ]);
		// co_await producer->enableTraceEvent([ "keyframe" ]);

		producer->on("trace", [=](json trace)
		{
			MSC_DEBUG(
				"producer \"trace\" event [producerId:%s, trace.type:%s, trace:%s]",
				producer->id().c_str(), trace["type"].get<std::string>().c_str(), trace.dump().c_str());
		});

		request->Accept(json{ { "id", producer->id() } });

		// Optimization: Create a server-side Consumer for each Peer.
		for (protoo::Peer* otherPeer : this->_getJoinedPeers(peer))
		{
			this->_createConsumer(otherPeer, peer, producer);
		}

		// Add into the audioLevelObserver.
		if (producer->kind() == "audio")
		{
			/*
			try
			{
				this->_audioLevelObserver.addProducer({ producerId: producer->id });
			}
			catch (const std::exception&)
			{

			}
			*/
		}
	}
	else if (method == "closeProducer")
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		std::string producerId = data["producerId"];

		if (!peer->data.producers.count(producerId))
			MSC_THROW_ERROR("producer with id \"%s\" not found", producerId.c_str());

		Producer* producer = peer->data.producers.at(producerId);

		producer->close();

		// Remove from its map.
		peer->data.producers.erase(producer->id());

		request->Accept(json::object());
	}
	else if (method == "pauseProducer")
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		std::string producerId = data["producerId"];

		if (!peer->data.producers.count(producerId))
			MSC_THROW_ERROR("producer with id \"%s\" not found", producerId.c_str());

		Producer* producer = peer->data.producers.at(producerId);

		co_await producer->pause();

		request->Accept(json::object());
	}
	else if (method == "resumeProducer")
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		std::string producerId = data["producerId"];

		if (!peer->data.producers.count(producerId))
			MSC_THROW_ERROR("producer with id \"%s\" not found", producerId.c_str());

		Producer* producer = peer->data.producers.at(producerId);

		co_await producer->resume();

		request->Accept(json::object());
	}
	else if (method == "pauseConsumer")
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		std::string consumerId = data["consumerId"];		

		if (!peer->data.consumers.count(consumerId))
			MSC_THROW_ERROR("consumer with id \"%s\" not found", consumerId.c_str());

		Consumer* consumer = peer->data.consumers.at(consumerId);

		co_await consumer->pause();

		request->Accept(json::object());
	}
	else if (method == "resumeConsumer")
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		std::string consumerId = data["consumerId"];

		if (!peer->data.consumers.count(consumerId))
			MSC_THROW_ERROR("consumer with id \"%s\" not found", consumerId.c_str());

		Consumer* consumer = peer->data.consumers.at(consumerId);

		co_await consumer->resume();

		request->Accept(json::object());
	}
	else if (method == "setConsumerPreferredLayers")
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		int spatialLayer = data["spatialLayer"];
		int temporalLayer = data["temporalLayer"];
		std::string consumerId = data["consumerId"];

		if (!peer->data.consumers.count(consumerId))
			MSC_THROW_ERROR("consumer with id \"%s\" not found", consumerId.c_str());

		Consumer* consumer = peer->data.consumers.at(consumerId);

		co_await consumer->setPreferredLayers(spatialLayer, temporalLayer);

		request->Accept(json::object());
	}
	else if (method == "setConsumerPriority")
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		int priority = data["priority"];
		std::string consumerId = data["consumerId"];

		if (!peer->data.consumers.count(consumerId))
			MSC_THROW_ERROR("consumer with id \"%s\" not found", consumerId.c_str());

		Consumer* consumer = peer->data.consumers.at(consumerId);

		co_await consumer->setPriority(priority);

		request->Accept(json::object());
	}
	else if (method == "requestConsumerKeyFrame")
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		std::string consumerId = data["consumerId"];

		if (!peer->data.consumers.count(consumerId))
			MSC_THROW_ERROR("consumer with id \"%s\" not found", consumerId.c_str());

		Consumer* consumer = peer->data.consumers.at(consumerId);

		co_await consumer->requestKeyFrame();

		request->Accept(json::object());
	}
	/*
	else if (method == "produceData")
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		const {
			transportId,
				sctpStreamParameters,
				label,
				protocol,
				appData
		} = request.data;

		const transport = peer->data.transports.get(transportId);

		if (!transport)
			MSC_THROW_ERROR("transport with id \"%s\" not found", transportId.c_str());

		const dataProducer = co_await transport->produceData(
			{
				sctpStreamParameters,
				label,
				protocol,
				appData
			});

		// Store the Producer into the protoo Peer data Object.
		peer->data.dataProducers.insert(dataProducer.id, dataProducer);

		request->Accept({ id: dataProducer.id });

		std::string label = dataProducer.label;

		if (label == "chat")
		{
			// Create a server-side DataConsumer for each Peer.
			for (const otherPeer of this->_getJoinedPeers(peer))
			{
				this->_createDataConsumer(
					{
						dataConsumerPeer: otherPeer,
						dataProducerPeer : peer,
						dataProducer
					});
			}
		}
		else if (label == "bot")
		{
			// Pass it to the bot.
			this->_bot.handlePeerDataProducer(
				{
					dataProducerId: dataProducer.id,
					peer
				});
		}		
	}
	*/
	else if (method == "changeDisplayName")
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		std::string displayName = data["displayName"];
		std::string oldDisplayName = peer->data.displayName;

		// Store the display name into the custom data Object of the protoo
		// Peer.
		peer->data.displayName = displayName;

		// Notify other joined Peers.
		for (protoo::Peer* otherPeer : this->_getJoinedPeers(peer))
		{
			try
			{
				otherPeer->notify(
					"peerDisplayNameChanged",
					json{
						{ "peerId", peer->id() },
						{ "displayName", displayName },
						{ "oldDisplayName", oldDisplayName }
					});
			}
			catch (const std::exception&)
			{

			}
		}

		request->Accept(json::object());
	}
	else if (method == "getTransportStats")
	{
		std::string transportId = data["transportId"];		

		if (!peer->data.transports.count(transportId))
			MSC_THROW_ERROR("transport with id \"%s\" not found", transportId.c_str());

		Transport* transport = peer->data.transports.at(transportId);

		json stats = co_await transport->getStats();

		request->Accept(stats);
	}
	else if (method == "getProducerStats")
	{
		std::string producerId = data["producerId"];

		if (!peer->data.producers.count(producerId))
			MSC_THROW_ERROR("producer with id \"%s\" not found", producerId.c_str());

		Producer* producer = peer->data.producers.at(producerId);

		json stats = co_await producer->getStats();

		request->Accept(stats);
	}
	else if (method == "getConsumerStats")
	{
		std::string consumerId = data["consumerId"];

		if (!peer->data.consumers.count(consumerId))
			MSC_THROW_ERROR("consumer with id \"%s\" not found", consumerId.c_str());

		Consumer* consumer = peer->data.consumers.at(consumerId);

		json stats = co_await consumer->getStats();

		request->Accept(stats);
	}
	/*
	else if (method == "getDataProducerStats")
	{
		const { dataProducerId } = request.data;
		const dataProducer = peer->data.dataProducers.get(dataProducerId);

		if (!dataProducer)
			MSC_THROW_ERROR(`dataProducer with id "${dataProducerId}" not found`);

		const stats = co_await dataProducer.getStats();

		request->Accept(stats);
	}
	else if (method == "getDataConsumerStats")
	{
		const { dataConsumerId } = request.data;
		const dataConsumer = peer->data.dataConsumers.get(dataConsumerId);

		if (!dataConsumer)
			MSC_THROW_ERROR(`dataConsumer with id "${dataConsumerId}" not found`);

		const stats = co_await dataConsumer.getStats();

		request->Accept(stats);
	}	
	else if (method == "applyNetworkThrottle")
	{
		const DefaultUplink = 1000000;
		const DefaultDownlink = 1000000;
		const DefaultRtt = 0;

		const { uplink, downlink, rtt, secret } = request.data;

		if (!secret || secret != process.env.NETWORK_THROTTLE_SECRET)
		{
			reject(403, "operation NOT allowed, modda fuckaa");

			return;
		}

		try
		{
			co_await throttle.start(
				{
					up: uplink || DefaultUplink,
					down : downlink || DefaultDownlink,
					rtt : rtt || DefaultRtt
				});

			MSC_WARN(
				"network throttle set [uplink:%s, downlink:%s, rtt:%s]",
				uplink || DefaultUplink,
				downlink || DefaultDownlink,
				rtt || DefaultRtt);

			request->Accept(json::object());
		}
		catch (std::exception& error)
		{
			MSC_ERROR("network throttle apply failed: %s", error.what());

			reject(500, error.toString());
		}
	}
	else if (method == "resetNetworkThrottle")
	{
		const { secret } = request.data;

		if (!secret || secret != process.env.NETWORK_THROTTLE_SECRET)
		{
			reject(403, "operation NOT allowed, modda fuckaa");

			return;
		}

		try
		{
			co_await throttle.stop({});

			MSC_WARN("network throttle stopped");

			request->Accept(json::object());
		}
		catch (std::exception& error)
		{
			MSC_ERROR("network throttle stop failed: %s", error.what());

			reject(500, error.toString());
		}
	}
	*/
	else
	{
		MSC_ERROR("unknown request.method \"%s\"", method.c_str());

		request->Reject(500, utils::Printf("unknown request.method \"%s\"",
			request->method.c_str()));
	}
}

std::list<protoo::Peer*> Room::_getJoinedPeers(protoo::Peer* excludePeer/* = nullptr*/)
{
	std::list<protoo::Peer*> peers;
	for (auto [key, peer] : this->_peers)
	{
		if (peer->data.joined && peer != excludePeer)
		{
			peers.push_back(peer);
		}
	}
	return peers;
}

/**
	 * Creates a mediasoup Consumer for the given mediasoup Producer.
	 *
	 * @async
	 */
std::future<void> Room::_createConsumer(protoo::Peer* consumerPeer, protoo::Peer* producerPeer, Producer* producer)
{
	// Optimization:
	// - Create the server-side Consumer in paused mode.
	// - Tell its Peer about it and wait for its response.
	// - Upon receipt of the response, resume the server-side Consumer.
	// - If video, this will mean a single key frame requested by the
	//   server-side Consumer (when resuming it).
	// - If audio (or video), it will avoid that RTP packets are received by the
	//   remote endpoint *before* the Consumer is locally created in the endpoint
	//   (and before the local SDP O/A procedure ends). If that happens (RTP
	//   packets are received before the SDP O/A is done) the PeerConnection may
	//   fail to associate the RTP stream.

	// NOTE: Don"t create the Consumer if the remote Peer cannot consume it.
	if (
		!consumerPeer->data.rtpCapabilities.is_object() ||
		!this->_mediasoupRouter->canConsume(producer->id(), consumerPeer->data.rtpCapabilities)
		)
	{
		return;
	}

	// Must take the Transport the remote Peer is using for consuming.
	Transport* transport = nullptr;
	for (auto [k,t] : consumerPeer->data.transports)
	{
		if (t->appData().value("consuming", false))
		{
			transport = t;
			break;
		}
	}

	// This should not happen.
	if (!transport)
	{
		MSC_WARN("_createConsumer() | Transport for consuming not found");

		return;
	}

	// Create the Consumer in paused mode.
	Consumer* consumer;

	try
	{
		ConsumerOptions options;
		options.producerId = producer->id();
		options.rtpCapabilities = consumerPeer->data.rtpCapabilities;
		options.paused = true;
		consumer = co_await transport->consume(options);
	}
	catch (std::exception& error)
	{
		MSC_WARN("_createConsumer() | transport.consume():%s", error.what());

		return;
	}

	// Store the Consumer into the protoo consumerPeer data Object.
	consumerPeer->data.consumers.insert(std::make_pair(consumer->id(), consumer));

	// Set Consumer events.
	consumer->on("transportclose", [=]()
	{
		// Remove from its map.
		consumerPeer->data.consumers.erase(consumer->id());
	});

	consumer->on("producerclose", [=]()
	{
		// Remove from its map.
		consumerPeer->data.consumers.erase(consumer->id());

		try
		{
			consumerPeer->notify("consumerClosed", json{ { "consumerId", consumer->id() } });
		}
		catch (const std::exception&)
		{

		}
	});

	consumer->on("producerpause", [=]()
	{
		try
		{
			consumerPeer->notify("consumerPaused", json{ { "consumerId", consumer->id() } });
		}
		catch (const std::exception&)
		{

		}
	});

	consumer->on("producerresume", [=]()
	{
		try
		{
			consumerPeer->notify("consumerResumed", json{ { "consumerId", consumer->id() } });
		}
		catch (const std::exception&)
		{

		}
	});

	consumer->on("score", [=](json& score)
	{
		// MSC_DEBUG(
		// 	"consumer "score" event [consumerId:%s, score:%o]",
		// 	consumer->id(), score);
		try
		{
			consumerPeer->notify("consumerScore", json{ { "consumerId", consumer->id() },{"score", score} });
		}
		catch (const std::exception&)
		{

		}
	});

	consumer->on("layerschange", [=](json& layers)
	{
		try
		{
			consumerPeer->notify(
				"consumerLayersChanged",
				json{
					{ "consumerId", consumer->id() },
					{ "spatialLayer",  !layers.is_null() ? layers["spatialLayer"] : json() },
					{ "temporalLayer",  !layers.is_null() ? layers["temporalLayer"] : json() }
				});
		}
		catch (const std::exception&)
		{

		}
	});

	// NOTE: For testing.
	// co_await consumer->enableTraceEvent([ "rtp", "keyframe", "nack", "pli", "fir" ]);
	// co_await consumer->enableTraceEvent([ "pli", "fir" ]);
	// co_await consumer->enableTraceEvent([ "keyframe" ]);

	consumer->on("trace", [=](json& trace)
	{
		MSC_DEBUG(
			"consumer \"trace\" event [producerId:%s, trace.type:%s, trace:%s]",
			consumer->id().c_str(), trace["type"].get<std::string>().c_str(), trace.dump().c_str());
	});

	// Send a protoo request to the remote Peer with Consumer parameters.
	try
	{
		co_await consumerPeer->request(
			"newConsumer",
			json{
				{ "peerId", producerPeer->id() },
				{ "producerId", producer->id() },
				{ "id", consumer->id() },
				{ "kind", consumer->kind() },
				{ "rtpParameters", consumer->rtpParameters() },
				{ "type", consumer->type() },
				{ "appData", producer->appData() },
				{ "producerPaused", consumer->producerPaused() }
			});

		// Now that we got the positive response from the remote endpoint, resume
		// the Consumer so the remote endpoint will receive the a first RTP packet
		// of this new stream once its PeerConnection is already ready to process
		// and associate it.
		co_await consumer->resume();

		try
		{
			consumerPeer->notify(
				"consumerScore",
				json{
					{ "consumerId", consumer->id() },
					{ "score", consumer->score() }
				});
		}
		catch (const std::exception&)
		{

		}
	}
	catch (std::exception& error)
	{
		MSC_WARN("_createConsumer() | failed:%s", error.what());
	}
}

void Room::OnPeerClose(protoo::Peer* peer)
{
	// 	DLOG(INFO) << "protoo Peer "close" event [peer:" << peer->id() << "]";
	// 
	// 	rs::Peer* mediaPeer = peer->mediaPeer();
	// 
	// 	if (mediaPeer && !mediaPeer->closed())
	// 		mediaPeer->close();

		// If this is the latest peer in the room, close the room.
		// However wait a bit (for reconnections).
	// 				setTimeout([=]()
	// 				{
	// 					if (this->_mediaRoom && this->_mediaRoom->closed())
	// 						return;
	// 
	// 					if (this->_mediaRoom->peers.length == 0)
	// 					{
	// 						DLOG(INFO) << 
	// 							"last peer in the room left, closing the room [roomId:"%s"]",
	// 							this->_roomId);
	// 
	// 						this->close();
	// 					}
	// 				}, 5000);
}

void Room::OnPeerRequest(protoo::Peer* peer, protoo::Request* request)
{
	MSC_DEBUG(
		"protoo Peer \"request\" event [method:%s, peerId:%s]",
		request->method.c_str(), peer->id().c_str());

	try
	{
		this->_handleProtooRequest(peer, request);
	}
	catch (std::exception& error)
	{
		MSC_ERROR("request failed:%s", error.what());

		request->Reject(500, error.what());
	}
}

void Room::OnPeerNotify(protoo::Peer* peer, json& notification)
{

}

void Room::_handleMediaRoom()
{
}

void Room::spread(std::string method, json data, std::set<std::string> excluded)
{
	for (auto it : this->_peers)
	{
		if (excluded.count(it.first))
			continue;

		// 		it.second->notify(method, data)
		// 			.fail([]() {});
	}
}

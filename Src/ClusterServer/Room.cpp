#define MSC_CLASS "Room"

#include "Room.hpp"
#include <Logger.hpp>
#include <errors.hpp>
#include <Worker.hpp>
#include <Router.hpp>
#include <Transport.hpp>
#include <Producer.hpp>
#include <Consumer.hpp>
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
	json mediaCodecs = R"(
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
				}
			]
)"_json;

	// Create a mediasoup Router.
	Router* mediasoupRouter = co_await mediasoupWorker->createRouter({ mediaCodecs });

	// Create a mediasoup AudioLevelObserver.
// 	const audioLevelObserver = await mediasoupRouter.createAudioLevelObserver(
// 		{
// 			maxEntries: 1,
// 			threshold : -80,
// 			interval : 800
// 		});
// 
// 	const bot = await Bot.create({ mediasoupRouter });

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

void Room::handleConnection(std::string peerId, bool consume, protoo::WebSocketClient* transport)
{
	if (this->_peers.count(peerId))
	{
		protoo::Peer* existingPeer = this->_peers.at(peerId);

		MSC_WARN(
			"handleProtooConnection() | there is already a protoo Peer with same peerId, closing it [peerId:%s]",
			peerId.c_str());

		existingPeer->close();
	}

	protoo::Peer* peer{nullptr};

	// Create a new protoo Peer with the given peerId.
	try
	{
		peer = new protoo::Peer(peerId, transport, this);
	}
	catch (std::exception& error)
	{
		MSC_ERROR("protooRoom.createPeer() failed:%s", error.what());
	}

	// Use the peer->data object to store mediasoup related objects.

	// Not joined after a custom protoo "join" request is later received.
	peer->data.consume = consume;
	peer->data.joined = false;

	peer->on("request", [=](request, accept, reject)
	{
		MSC_DEBUG(
			"protoo Peer \"request\" event [method:%s, peerId:%s]",
			request.method.c_str(), peer->id().c_str());

		try
		{
			this->_handleProtooRequest(peer, request, accept, reject);
		}
		catch (std::exception& error)
		{
			MSC_ERROR("request failed:%s", error.what());

			reject(error);
		}
	});

	peer->on("close", [=]() 
	{
		if (this->_closed)
			return;

		MSC_DEBUG("protoo Peer \"close\" event [peerId:%s]", peer->id().c_str());

		// If the Peer was joined, notify all Peers.
		if (peer->data.joined)
		{
			for (protoo::Peer* otherPeer : this->_getJoinedPeers(peer))
			{
				otherPeer->notify("peerClosed", { peerId: peer->id() })
					.catch (() = > {});
			}
		}

		// Iterate and close all mediasoup Transport associated to this Peer, so all
		// its Producers and Consumers will also be closed.
		for (auto [key, transport] : peer->data.transports)
		{
			transport->close();
		}

		// If this is the latest Peer in the room, close the room.
		if (this->_protooRoom.peers.length == 0)
		{
			MSC_DEBUG(
				"last Peer in the room left, closing the room [roomId:%s]",
				this->_roomId.c_str());

			this->close();
		}
	});
}

/**
	 * Handle protoo requests from browsers.
	 *
	 * @async
	 */
async _handleProtooRequest(protoo::Peer* peer, request, accept, reject)
{
	std::string method = request.method;

	if( method == "getRouterRtpCapabilities" )
	{
		accept(this->_mediasoupRouter->rtpCapabilities());
	}
	else if( method == "join" )
	{
		// Ensure the Peer is not already joined.
		if (peer->data.joined)
			MSC_THROW_ERROR("Peer already joined");

		json {
			displayName,
				device,
				rtpCapabilities,
				sctpCapabilities
		} = request.data;

		// Store client data into the protoo Peer data object.
		peer->data.joined = true;
		peer->data.displayName = displayName;
		peer->data.device = device;
		peer->data.rtpCapabilities = rtpCapabilities;
		peer->data.sctpCapabilities = sctpCapabilities;

		// Tell the new Peer about already joined Peers.
		// And also create Consumers for existing Producers.

		std::list<protoo::Peer*> joinedPeers =
			[
				...this->_getJoinedPeers(),
				...this->_broadcasters.values()
			];

		// Reply now the request with the list of joined peers (all but the new one).
		json peerInfos = joinedPeers
			.filter((joinedPeer) = > joinedPeer.id != peer->id())
			.map((joinedPeer) = > ({
				id: joinedPeer.id,
				displayName : joinedPeer.data.displayName,
				device : joinedPeer.data.device
				}));

		accept(json{ { "peers", peerInfos } });

		// Mark the new Peer as joined.
		peer->data.joined = true;

		for (const joinedPeer : joinedPeers)
		{
			// Create Consumers for existing Producers.
			for (const producer of joinedPeer.data.producers.values())
			{
				this->_createConsumer(
					{
						consumerPeer: peer,
						producerPeer : joinedPeer,
						producer
					});
			}

			// Create DataConsumers for existing DataProducers.
			for (const dataProducer of joinedPeer.data.dataProducers.values())
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
		}

		// Create DataConsumers for bot DataProducer.
		this->_createDataConsumer(
			{
				dataConsumerPeer: peer,
				dataProducerPeer : null,
				dataProducer : this->_bot.dataProducer
			});

		// Notify the new Peer to all other Peers.
		for (protoo::Peer* otherPeer : this->_getJoinedPeers(peer))
		{
			otherPeer->notify(
				"newPeer",
				{
					id: peer->id,
					displayName : peer->data.displayName,
					device : peer->data.device
				})
				.catch (() = > {});
		}
	}
	else if( method == "createWebRtcTransport" )
	{
		// NOTE: Don"t require that the Peer is joined here, so the client can
		// initiate mediasoup Transports and be ready when he later joins.

		json {
			forceTcp,
				producing,
				consuming,
				sctpCapabilities
		} = request.data;

		const webRtcTransportOptions =
		{
			...config.mediasoup.webRtcTransportOptions,
			enableSctp     : Boolean(sctpCapabilities),
			numSctpStreams : (sctpCapabilities || {}).numStreams,
			appData : { producing, consuming }
		};

		if (forceTcp)
		{
			webRtcTransportOptions.enableUdp = false;
			webRtcTransportOptions.enableTcp = true;
		}

		WebRtcTransport* transport = await this->_mediasoupRouter->createWebRtcTransport(
			webRtcTransportOptions);

		transport->on("sctpstatechange", (sctpState) = >
		{
			MSC_DEBUG("WebRtcTransport "sctpstatechange" event [sctpState:%s]", sctpState);
		});

		transport->on("dtlsstatechange", (dtlsState) = >
		{
			if (dtlsState == "failed" || dtlsState == "closed")
				MSC_WARN("WebRtcTransport "dtlsstatechange" event [dtlsState:%s]", dtlsState);
		});

		// NOTE: For testing.
		// await transport->enableTraceEvent([ "probation", "bwe" ]);
		await transport->enableTraceEvent(["bwe"]);

		transport->on("trace", [=](trace)
		{
			MSC_DEBUG(
				"transport "trace" event [transportId:%s, trace.type:%s, trace:%o]",
				transport->id(), trace.type, trace);

			if (trace.type == "bwe" && trace.direction == "out")
			{
				peer->notify(
					"downlinkBwe",
					{
						desiredBitrate: trace.info.desiredBitrate,
						effectiveDesiredBitrate : trace.info.effectiveDesiredBitrate,
						availableBitrate : trace.info.availableBitrate
					})
					.catch (() = > {});
			}
		});

		// Store the WebRtcTransport into the protoo Peer data Object.
		peer->data.transports.insert(transport->id(), transport);

		accept(
			{
				id: transport->id(),
				iceParameters : transport->iceParameters,
				iceCandidates : transport->iceCandidates,
				dtlsParameters : transport->dtlsParameters,
				sctpParameters : transport->sctpParameters
			});

		const { maxIncomingBitrate } = config.mediasoup.webRtcTransportOptions;

		// If set, apply max incoming bitrate limit.
		if (maxIncomingBitrate)
		{
			try { 
				await transport->setMaxIncomingBitrate(maxIncomingBitrate); 
			}
			catch (std::exception& error)
			{
			}
		}
	}
	else if( method == "connectWebRtcTransport" )
	{
		const { transportId, dtlsParameters } = request.data;
		const transport = peer->data.transports.get(transportId);

		if (!transport)
			MSC_THROW_ERROR("transport with id \"%s\" not found", transportId.c_str());

		await transport->connect({ dtlsParameters });

		accept();
	}
	else if( method == "restartIce" )
	{
		const { transportId } = request.data;
		const transport = peer->data.transports.get(transportId);

		if (!transport)
			MSC_THROW_ERROR("transport with id \"%s\" not found", transportId.c_str());

		const iceParameters = await transport->restartIce();

		accept(iceParameters);
	}
	else if( method == "produce" )
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		const { transportId, kind, rtpParameters } = request.data;
		let{ appData } = request.data;
		const transport = peer->data.transports.get(transportId);

		if (!transport)
			MSC_THROW_ERROR("transport with id \"%s\" not found", transportId.c_str());

		// Add peerId into appData to later get the associated Peer during
		// the "loudest" event of the audioLevelObserver.
		appData = { ...appData, peerId: peer->id };

		const producer = await transport->produce(
			{
				kind,
				rtpParameters,
				appData
				// keyFrameRequestDelay: 5000
			});

		// Store the Producer into the protoo Peer data Object.
		peer->data.producers.set(producer->id, producer);

		// Set Producer events.
		producer->on("score", (score) = >
		{
			// MSC_DEBUG(
			// 	"producer "score" event [producerId:%s, score:%o]",
			// 	producer->id, score);

			peer->notify("producerScore", { producerId: producer->id, score })
				.catch (() = > {});
		});

		producer->on("videoorientationchange", (videoOrientation) = >
		{
			MSC_DEBUG(
				"producer "videoorientationchange" event [producerId:%s, videoOrientation:%o]",
				producer->id, videoOrientation);
		});

		// NOTE: For testing.
		// await producer->enableTraceEvent([ "rtp", "keyframe", "nack", "pli", "fir" ]);
		// await producer->enableTraceEvent([ "pli", "fir" ]);
		// await producer->enableTraceEvent([ "keyframe" ]);

		producer->on("trace", (trace) = >
		{
			MSC_DEBUG(
				"producer "trace" event [producerId:%s, trace.type:%s, trace:%o]",
				producer->id, trace.type, trace);
		});

		accept({ id: producer->id });

		// Optimization: Create a server-side Consumer for each Peer.
		for (protoo::Peer* otherPeer : this->_getJoinedPeers({ excludePeer: peer }))
		{
			this->_createConsumer(
				{
					consumerPeer: otherPeer,
					producerPeer : peer,
					producer
				});
		}

		// Add into the audioLevelObserver.
		if (producer->kind() == "audio")
		{
			this->_audioLevelObserver.addProducer({ producerId: producer->id })
				.catch (() = > {});
		}
	}
	else if( method == "closeProducer" )
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		const { producerId } = request.data;
		const producer = peer->data.producers.get(producerId);

		if (!producer)
			MSC_THROW_ERROR("producer with id \"%s\" not found", producerId.c_str());

		producer->close();

		// Remove from its map.
		peer->data.producers.delete(producer->id);

		accept();
	}
	else if( method == "pauseProducer" )
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		const { producerId } = request.data;
		const producer = peer->data.producers.get(producerId);

		if (!producer)
			MSC_THROW_ERROR("producer with id \"%s\" not found", producerId.c_str());

		await producer->pause();

		accept();
	}
	else if( method == "resumeProducer" )
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		const { producerId } = request.data;
		const producer = peer->data.producers.get(producerId);

		if (!producer)
			MSC_THROW_ERROR("producer with id \"%s\" not found", producerId.c_str());

		await producer->resume();

		accept();
	}
	else if( method == "pauseConsumer" )
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		const { consumerId } = request.data;
		const consumer = peer->data.consumers.get(consumerId);

		if (!consumer)
			MSC_THROW_ERROR("consumer with id \"%s\" not found", consumerId.c_str());

		await consumer.pause();

		accept();
	}
	else if( method == "resumeConsumer" )
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		const { consumerId } = request.data;
		const consumer = peer->data.consumers.get(consumerId);

		if (!consumer)
			MSC_THROW_ERROR("consumer with id \"%s\" not found", consumerId.c_str());

		await consumer.resume();

		accept();
	}
	else if( method == "setConsumerPreferredLayers" )
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		const { consumerId, spatialLayer, temporalLayer } = request.data;
		const consumer = peer->data.consumers.get(consumerId);

		if (!consumer)
			MSC_THROW_ERROR("consumer with id \"%s\" not found", consumerId.c_str());

		await consumer.setPreferredLayers({ spatialLayer, temporalLayer });

		accept();
	}
	else if( method == "setConsumerPriority" )
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		const { consumerId, priority } = request.data;
		const consumer = peer->data.consumers.get(consumerId);

		if (!consumer)
			MSC_THROW_ERROR("consumer with id \"%s\" not found", consumerId.c_str());

		await consumer.setPriority(priority);

		accept();
	}
	else if( method == "requestConsumerKeyFrame" )
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		const { consumerId } = request.data;
		const consumer = peer->data.consumers.get(consumerId);

		if (!consumer)
			MSC_THROW_ERROR("consumer with id \"%s\" not found", consumerId.c_str());

		await consumer.requestKeyFrame();

		accept();
	}
	else if( method == "produceData" )
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

		const dataProducer = await transport->produceData(
			{
				sctpStreamParameters,
				label,
				protocol,
				appData
			});

		// Store the Producer into the protoo Peer data Object.
		peer->data.dataProducers.insert(dataProducer.id, dataProducer);

		accept({ id: dataProducer.id });

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
	else if( method == "changeDisplayName" )
	{
		// Ensure the Peer is joined.
		if (!peer->data.joined)
			MSC_THROW_ERROR("Peer not yet joined");

		const { displayName } = request.data;
		const oldDisplayName = peer->data.displayName;

		// Store the display name into the custom data Object of the protoo
		// Peer.
		peer->data.displayName = displayName;

		// Notify other joined Peers.
		for (protoo::Peer* otherPeer : this->_getJoinedPeers(peer))
		{
			otherPeer->notify(
				"peerDisplayNameChanged",
				{
					peerId: peer->id,
					displayName,
					oldDisplayName
				})
				.catch (() = > {});
		}

		accept();
	}
	else if( method == "getTransportStats" )
	{
		const { transportId } = request.data;
		const transport = peer->data.transports.get(transportId);

		if (!transport)
			MSC_THROW_ERROR("transport with id \"%s\" not found", transportId.c_str());

		const stats = await transport->getStats();

		accept(stats);
	}
	else if( method == "getProducerStats" )
	{
		const { producerId } = request.data;
		const producer = peer->data.producers.get(producerId);

		if (!producer)
			MSC_THROW_ERROR("producer with id \"%s\" not found", producerId.c_str());

		const stats = await producer->getStats();

		accept(stats);
	}
	else if( method == "getConsumerStats" )
	{
		const { consumerId } = request.data;
		const consumer = peer->data.consumers.get(consumerId);

		if (!consumer)
			MSC_THROW_ERROR("consumer with id \"%s\" not found", consumerId.c_str());

		const stats = await consumer.getStats();

		accept(stats);
	}
	else if( method == "getDataProducerStats" )
	{
		const { dataProducerId } = request.data;
		const dataProducer = peer->data.dataProducers.get(dataProducerId);

		if (!dataProducer)
			MSC_THROW_ERROR(`dataProducer with id "${dataProducerId}" not found`);

		const stats = await dataProducer.getStats();

		accept(stats);
	}
	else if( method == "getDataConsumerStats" )
	{
		const { dataConsumerId } = request.data;
		const dataConsumer = peer->data.dataConsumers.get(dataConsumerId);

		if (!dataConsumer)
			MSC_THROW_ERROR(`dataConsumer with id "${dataConsumerId}" not found`);

		const stats = await dataConsumer.getStats();

		accept(stats);
	}
	else if( method == "applyNetworkThrottle" )
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
			await throttle.start(
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

			accept();
		}
		catch (std::exception& error)
		{
			MSC_ERROR("network throttle apply failed: %s", error.what());

			reject(500, error.toString());
		}
	}
	else if( method == "resetNetworkThrottle" )
	{
		const { secret } = request.data;

		if (!secret || secret != process.env.NETWORK_THROTTLE_SECRET)
		{
			reject(403, "operation NOT allowed, modda fuckaa");

			return;
		}

		try
		{
			await throttle.stop({});

			MSC_WARN("network throttle stopped");

			accept();
		}
		catch (std::exception& error)
		{
			MSC_ERROR("network throttle stop failed: %s", error.what());

			reject(500, error.toString());
		}
	}
	else
	{
		MSC_ERROR("unknown request.method \"%s\"", request.method);

		reject(500, "unknown request.method \"${request.method}\"");
	}
}

std::list<protoo::Peer*> Room::_getJoinedPeers(protoo::Peer* excludePeer = nullptr)
{
	std::list<protoo::Peer*> peers;
	for (protoo::Peer* peer : this->_peers)
	{
		if (peer->data.joined && peer != excludePeer)
		{
			peers.push_back(peer);
		}
	}
	return peers;
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
// 				setTimeout(() = >
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

void Room::OnPeerRequest(protoo::Peer* peer, json& request)
{
	uint32_t id = request.value("id", 0);
	std::string method = request["method"].get<std::string>();

// 	DLOG(INFO) << "protoo "request" event [method:" << method << "peer:" << peer->id() << "]";
// 
// 	if (method == "mediasoup-request")
// 	{
// 		json mediasoupRequest = request["data"];
// 
// 		this->_handleMediasoupClientRequest(peer, id, mediasoupRequest);
// 	}
// 	else if (method == "mediasoup-notification")
// 	{
// 		peer->Accept(id, json::object());
// 
// 		json mediasoupNotification = request["data"];
// 
// 		this->_handleMediasoupClientNotification(
// 			peer, mediasoupNotification);
// 	}
// 	else if (method == "change-display-name")
// 	{
// 		peer->Accept(id, json::object());
// 
// 		std::string displayName = request["displayName"].get<std::string>();;
// 		rs::Peer* mediaPeer = peer->mediaPeer();
// 		std::string oldDisplayName = mediaPeer->appData()["displayName"].get<std::string>();
// 
// 		mediaPeer->appData()["displayName"] = displayName;
// 
// 		// Spread to others via protoo.
// 		spread("display-name-changed",
// 			{
// 				{ "peerName" , peer->id() },
// 				{ "displayName" , displayName },
// 				{ "oldDisplayName" , oldDisplayName }
// 			},
// 			{ peer->id() }
// 		);
// 	}
// 	else
// 	{
// 		LOG(ERROR) << "unknown request.method:" << method;
// 
// 		peer->Reject(id, 400, "unknown request.method " + method);
// 	}
}

void Room::OnPeerNotify(protoo::Peer* peer, json& notification)
{

}

void Room::_handleMediaRoom()
{
// 	DLOG(INFO) << "_handleMediaRoom()";
// 
// 	
// 	auto activeSpeakerDetector = this->_mediaRoom->createActiveSpeakerDetector();
// 
// 	activeSpeakerDetector->addEventListener("activespeakerchange", [=](json jActivePeerName)
// 	{
// 		rs::Peer* activePeer = nullptr;
// 		if (!jActivePeerName.is_null())
// 		{
// 			std::string activePeerName = jActivePeerName.get<std::string>();
// 			activePeer = this->_mediaRoom->getPeerByName(activePeerName);
// 		}
// 
// 		if (activePeer)
// 		{
// 			DLOG(INFO) << "new active speaker [peerName:"" << activePeer->name() << ""]";
// 
// 			//this->_currentActiveSpeaker = activePeer;
// 
// 			rs::Producers producers;
// 			for (auto itProducer : activePeer->producers())
// 			{
// 				auto producer = itProducer.second;
// 				if (producer->kind() == "video")
// 				{
// 					producers.insert(std::make_pair(producer->id(), producer));
// 				}
// 			}
// 
// 
// 			for (const auto itPeer : this->_mediaRoom->peers())
// 			{
// 				rs::Peer* peer = itPeer.second;
// 				for (const auto itConsumer : peer->consumers())
// 				{
// 					rs::Consumer* consumer = itConsumer.second;
// 
// 					if (consumer->kind() != "video")
// 						continue;
// 
// 					if (producers.count(consumer->source()->id()))
// 					{
// 						//consumer->setPreferredProfile("high");
// 					}
// 					else
// 					{
// 						//consumer->setPreferredProfile("low");
// 					}
// 				}
// 			}
// 		}
// 		else
// 		{
// 			DLOG(INFO) << "no active speaker";
// 
// 			//this->_currentActiveSpeaker = nullptr;
// 
// 			for (const auto itPeer : this->_mediaRoom->peers())
// 			{
// 				rs::Peer* peer = itPeer.second;
// 				for (const auto itConsumer : peer->consumers())
// 				{
// 					rs::Consumer* consumer = itConsumer.second;
// 					if (consumer->kind() != "video")
// 						continue;
// 
// 					//consumer->setPreferredProfile("low");
// 				}
// 			}
// 		}
// 
// 		// Spread to others via protoo.
// 		spread(
// 			"active-speaker",
// 			{
// 				{ "peerName" , activePeer ? activePeer->name() : "" }
// 			});
// 	});
}

// void Room::_handleMediaPeer(protoo::Peer* protooPeer, rs::Peer* mediaPeer)
// {
// 	mediaPeer->addEventListener("notify", [=](json notification)
// 	{
// 		protooPeer->send("mediasoup-notification", notification)
// 			.fail([]() {});
// 	});
// 
// 	mediaPeer->addEventListener("newtransport", [=](json data)
// 	{
// 		rs::WebRtcTransport* transport = mediaPeer->getTransportById(data.get<uint32_t>());
// 
// 		DLOG(INFO) << 
// 			"mediaPeer "newtransport" event [id:"<< transport->id()()<< " direction:"
// 			<< transport->direction() << "]";
// 
// 		// Update peers max sending  bitrate.
// 		if (transport->direction() == "send")
// 		{
// 			this->_updateMaxBitrate();
// 
// 			transport->addEventListener("close", [=](json data)
// 			{
// 				this->_updateMaxBitrate();
// 			});
// 		}
// 
// 		this->_handleMediaTransport(transport);
// 	});
// 
// 	mediaPeer->addEventListener("newproducer", [=](json data)
// 	{
// 		rs::Producer* producer = mediaPeer->getProducerById(data.get<uint32_t>());
// 		DLOG(INFO) << "mediaPeer "newproducer" event [id:" << producer->id() << "]";
// 
// 		this->_handleMediaProducer(producer);
// 	});
// 
// 	mediaPeer->addEventListener("newconsumer", [=](json data)
// 	{
// 		rs::Consumer* consumer = mediaPeer->getConsumerById(data);
// 		DLOG(INFO) << "mediaPeer "newconsumer" event [id:" << consumer->id() << "]";
// 
// 		this->_handleMediaConsumer(consumer);
// 	});
// 
// 	// Also handle already existing Consumers.
// 	for (auto itConsumer : mediaPeer->consumers())
// 	{
// 		auto consumer = itConsumer.second;
// 
// 		DLOG(INFO) << "mediaPeer existing "consumer" [id:" << consumer->id() << "]";
// 
// 		this->_handleMediaConsumer(consumer);
// 	}
// }
// 
// void Room::_handleMediaTransport(rs::WebRtcTransport* transport)
// {
// 	transport->addEventListener("close", [=](json originator)
// 	{
// 		DLOG(INFO) << 
// 			"Transport "close" event [originator:" << originator.dump() << "]";
// 	});
// }
// 
// void Room::_handleMediaProducer(rs::Producer* producer)
// {
// 	producer->addEventListener("close", [=](json originator)
// 	{
// 		DLOG(INFO) << 
// 			"Producer "close" event [originator:" << originator.dump() << "]";
// 	});
// 
// 	producer->addEventListener("pause", [=](json originator)
// 	{
// 		DLOG(INFO) << 
// 			"Producer "pause" event [originator:" << originator.dump() << "]";
// 	});
// 
// 	producer->addEventListener("resume", [=](json originator)
// 	{
// 		DLOG(INFO) << 
// 			"Producer "resume" event [originator:" << originator.dump() << "]";
// 	});
// }
// 
// void Room::_handleMediaConsumer(rs::Consumer* consumer)
// {
// 	consumer->addEventListener("close", [=](json originator)
// 	{
// 		DLOG(INFO) << 
// 			"Consumer "close" event [originator:" << originator.dump() << "]";
// 	});
// 
// 	consumer->addEventListener("pause", [=](json originator)
// 	{
// 		DLOG(INFO) << 
// 			"Consumer "pause" event [originator:" << originator.dump() << "]";
// 	});
// 
// 	consumer->addEventListener("resume", [=](json originator)
// 	{
// 		DLOG(INFO) << 
// 			"Consumer "resume" event [originator:" << originator.dump() << "]";
// 	});
// 
// 	consumer->addEventListener("effectiveprofilechange", [=](json profile)
// 	{
// 		DLOG(INFO) << 
// 			"Consumer "effectiveprofilechange" event [profile:" << profile.dump() << "]";
// 	});
// 
// 	// If video, initially make it "low" profile unless this is for the current
// 	// active speaker.
// // 	if (consumer->kind() == "video" && consumer->peer() != this->_currentActiveSpeaker)
// // 		consumer->setPreferredProfile("low");
// }
// 
// void Room::_handleMediasoupClientRequest(protoo::Peer* protooPeer, uint32_t id, json request)
// {
// 	std::string method = request["method"].get<std::string>();
// 
// 	DLOG(INFO) <<
// 		"mediasoup-client request [method:" << method << ", peer:"" << protooPeer->id() << ""]";
// 
// 	if (method == "queryRoom")
// 	{
// 		this->_mediaRoom->receiveRequest(request)
// 		.then([=](json response)
// 		{
// 			protooPeer->Accept(id, response);
// 		})
// 		.fail([=](rs::Error error)
// 		{
// 			protooPeer->Reject(id, 500, error.ToString());
// 		});
// 	}
// 	else if (method == "join")
// 	{
// 		// TODO: Handle appData. Yes?
// 		std::string peerName = request["peerName"].get<std::string>();
// 
// 		if (peerName != protooPeer->id())
// 		{
// 			protooPeer->Reject(id, 403, "that is not your corresponding mediasoup Peer name");
// 		}
// 		else if (protooPeer->mediaPeer())
// 		{
// 			protooPeer->Reject(id, 500, "already have a mediasoup Peer");
// 		}
// 
// 		this->_mediaRoom->receiveRequest(request)
// 		.then([=](json response)
// 		{
// 			protooPeer->Accept(id, response);
// 
// 			// Get the newly created mediasoup Peer.
// 			rs::Peer* mediaPeer = this->_mediaRoom->getPeerByName(peerName);
// 
// 			protooPeer->setMediaPeer(mediaPeer);
// 
// 			this->_handleMediaPeer(protooPeer, mediaPeer);
// 		})
// 		.fail([=](rs::Error error)
// 		{
// 			protooPeer->Reject(id, 500, error.ToString());
// 		});
// 	}
// 	else
// 	{
// 		rs::Peer* mediaPeer = protooPeer->mediaPeer();
// 
// 		if (!mediaPeer)
// 		{
// 			LOG(ERROR) << "cannot handle mediasoup request, no mediasoup Peer [method:"
// 				<< method << "]";
// 
// 			protooPeer->Reject(id, 400, "no mediasoup Peer");
// 		}
// 
// 		mediaPeer->receiveRequest(request)
// 		.then([=](json response)
// 		{
// 			protooPeer->Accept(id, response);
// 		})
// 		.fail([=](rs::Error error)
// 		{
// 			protooPeer->Reject(id, 500, error.ToString().c_str());
// 		});
// 	}
// }
// 
// void Room::_handleMediasoupClientNotification(protoo::Peer* protooPeer, json notification)
// {
// 	std::string method = notification["method"].get<std::string>();
// 
// 	DLOG(INFO) <<
// 		"mediasoup-client notification [method:" << method << ", peer:" << protooPeer->id() << "]";
// 
// 	// NOTE: mediasoup-client just sends notifications with target "peer",
// 	// so first of all, get the mediasoup Peer.
// 	rs::Peer* mediaPeer = protooPeer->mediaPeer();
// 
// 	if (!mediaPeer)
// 	{
// 		LOG(ERROR) <<
// 			"cannot handle mediasoup notification, no mediasoup Peer [method:" << method << "]";
// 		return;
// 	}
// 
// 	mediaPeer->receiveNotification(notification);
// }
// 
// void Room::_updateMaxBitrate()
// {
// 	if (this->_mediaRoom->closed())
// 		return;
// 
// 	int numPeers = this->_mediaRoom->peers().size();
// 	uint32_t previousMaxBitrate = this->_maxBitrate;
// 	uint32_t newMaxBitrate;
// 
// 	if (numPeers <= 2)
// 	{
// 		newMaxBitrate = MAX_BITRATE;
// 	}
// 	else
// 	{
// 		newMaxBitrate = std::round(MAX_BITRATE / ((numPeers - 1) * BITRATE_FACTOR));
// 
// 		if (newMaxBitrate < MIN_BITRATE)
// 			newMaxBitrate = MIN_BITRATE;
// 	}
// 
// 	this->_maxBitrate = newMaxBitrate;
// 
// 	for (auto itPeer : this->_mediaRoom->peers())
// 	{
// 		auto peer = itPeer.second;
// 		for (auto itTransport : peer->transports())
// 		{
// 			auto transport = itTransport.second;
// 			if (transport->direction() == "send")
// 			{
// 				transport->setMaxBitrate(newMaxBitrate)
// 					.fail([=](std::string error)
// 				{
// 					LOG(ERROR) << "transport->setMaxBitrate() failed:" << error.c_str();
// 				});
// 			}
// 		}
// 	}
// 
// 	DLOG(INFO) <<
// 		"_updateMaxBitrate() [num peers:" << numPeers
// 		<< ", before:" << std::round(previousMaxBitrate / 1000)
// 		<< "kbps, now:" << std::round(newMaxBitrate / 1000) << "kbps]";
// }

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

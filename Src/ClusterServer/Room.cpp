#define MS_CLASS "Room"

#include "Room.hpp"
#include "Utils.hpp"
#include "WebSocketClient.hpp"
#include <math.h>

const uint32_t MAX_BITRATE = 3000000;
const uint32_t MIN_BITRATE = 50000;
const float BITRATE_FACTOR = 0.75;

Room::Room(std::string roomId, Json mediaCodecs, rs::Server* mediaServer, Listener* listener)
	: _roomId(roomId)
	, _mediaServer(mediaServer)
	, listener(listener)
{
	// mediasoup Room instance.
	this->_mediaRoom = mediaServer->Room(mediaCodecs);

	_maxBitrate = MAX_BITRATE;

	this->_handleMediaRoom();
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
	if (this->_mediaRoom)
		this->_mediaRoom->close();

	// Emit "close" event.
	this->listener->OnRoomClose(_roomId);
}

void Room::handleConnection(std::string peerName, protoo::WebSocketClient* transport)
{
	DLOG(INFO) << "handleConnection() [peerName:" << peerName << "]";

	if (this->_peers.count(peerName))
	{
		LOG(WARNING) <<
			"handleConnection() | there is already a peer with same peerName, \
			closing the previous one [peerName:" << peerName << "]";

		auto protooPeer = this->_peers[peerName];

		protooPeer->close();
	}

	protoo::Peer* protooPeer = new protoo::Peer(peerName, transport, this);

	this->_peers[peerName] = protooPeer;
}

void Room::OnPeerClose(protoo::Peer* peer)
{
	DLOG(INFO) << "protoo Peer 'close' event [peer:" << peer->id() << "]";

	rs::Peer* mediaPeer = peer->mediaPeer();

	if (mediaPeer && !mediaPeer->closed())
		mediaPeer->close();

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
// 							"last peer in the room left, closing the room [roomId:'%s']",
// 							this->_roomId);
// 
// 						this->close();
// 					}
// 				}, 5000);
}

void Room::OnPeerRequest(protoo::Peer* peer, Json& request)
{
	uint32_t id = request.value("id", 0);
	std::string method = request["method"].get<std::string>();

	DLOG(INFO) << "protoo 'request' event [method:" << method << "peer:" << peer->id() << "]";

	if (method == "mediasoup-request")
	{
		Json mediasoupRequest = request["data"];

		this->_handleMediasoupClientRequest(peer, id, mediasoupRequest);
	}
	else if (method == "mediasoup-notification")
	{
		peer->Accept(id, Json::object());

		Json mediasoupNotification = request["data"];

		this->_handleMediasoupClientNotification(
			peer, mediasoupNotification);
	}
	else if (method == "change-display-name")
	{
		peer->Accept(id, Json::object());

		std::string displayName = request["displayName"].get<std::string>();;
		rs::Peer* mediaPeer = peer->mediaPeer();
		std::string oldDisplayName = mediaPeer->appData()["displayName"].get<std::string>();

		mediaPeer->appData()["displayName"] = displayName;

		// Spread to others via protoo.
		spread("display-name-changed",
			{
				{ "peerName" , peer->id() },
				{ "displayName" , displayName },
				{ "oldDisplayName" , oldDisplayName }
			},
			{ peer->id() }
		);
	}
	else
	{
		LOG(ERROR) << "unknown request.method:" << method;

		peer->Reject(id, 400, "unknown request.method " + method);
	}
}

void Room::OnPeerNotify(protoo::Peer* peer, Json& notification)
{

}

void Room::_handleMediaRoom()
{
	DLOG(INFO) << "_handleMediaRoom()";

	/*
	auto activeSpeakerDetector = this->_mediaRoom->createActiveSpeakerDetector();

	activeSpeakerDetector->on("activespeakerchange", (activePeer) = >
	{
		if (activePeer)
		{
			DLOG(INFO) << "new active speaker [peerName:'%s']", activePeer.name);

			this->_currentActiveSpeaker = activePeer;

			const activeVideoProducer = activePeer.producers
				.find((producer) = > producer->kind == "video");

			for (const peer of this->_mediaRoom->peers)
			{
				for (const consumer of peer.consumers)
				{
					if (consumer->kind != = "video")
						continue;

					if (consumer->source == activeVideoProducer)
					{
						consumer->setPreferredProfile("high");
					}
					else
					{
						consumer->setPreferredProfile("low");
					}
				}
			}
		}
		else
		{
			DLOG(INFO) << "no active speaker");

			this->_currentActiveSpeaker = null;

			for (const peer of this->_mediaRoom->peers)
			{
				for (const consumer of peer.consumers)
				{
					if (consumer->kind != = "video")
						continue;

					consumer->setPreferredProfile("low");
				}
			}
		}

		// Spread to others via protoo.
		spread(
			"active-speaker",
			{
			peerName: activePeer ? activePeer.name : null
			});
	});
	*/

}

void Room::_handleMediaPeer(protoo::Peer* protooPeer, rs::Peer* mediaPeer)
{
	mediaPeer->addEventListener("notify", [=](Json notification)
	{
		protooPeer->send("mediasoup-notification", notification)
			.fail([]() {});
	});

	mediaPeer->addEventListener("newtransport", [=](Json data)
	{
		rs::WebRtcTransport* transport = mediaPeer->getTransportById(data.get<uint32_t>());

		DLOG(INFO) << 
			"mediaPeer 'newtransport' event [id:"<< transport->id()<< "direction:"
			<< transport->direction() << "]";

		// Update peers max sending  bitrate.
		if (transport->direction() == "send")
		{
			this->_updateMaxBitrate();

			transport->addEventListener("close", [=](Json data)
			{
				this->_updateMaxBitrate();
			});
		}

		this->_handleMediaTransport(transport);
	});

	mediaPeer->addEventListener("newproducer", [=](Json data)
	{
		rs::Producer* producer = mediaPeer->getProducerById(data.get<uint32_t>());
		DLOG(INFO) << "mediaPeer 'newproducer' event [id:" << producer->id() << "]";

		this->_handleMediaProducer(producer);
	});

	mediaPeer->addEventListener("newconsumer", [=](Json data)
	{
		rs::Consumer* consumer = mediaPeer->getConsumerById(data);
		DLOG(INFO) << "mediaPeer 'newconsumer' event [id:" << consumer->id() << "]";

		this->_handleMediaConsumer(consumer);
	});

	// Also handle already existing Consumers.
	for (auto itConsumer : mediaPeer->consumers())
	{
		auto consumer = itConsumer.second;

		DLOG(INFO) << "mediaPeer existing 'consumer' [id:" << consumer->id() << "]";

		this->_handleMediaConsumer(consumer);
	}

	// Notify about the existing active speaker.
	if (this->_currentActiveSpeaker)
	{
		// 		protooPeer->send(
		// 			"active-speaker",
		// 			{
		// 				{"peerName" , this->_currentActiveSpeaker.name}
		// 			})
		// 			.catch (() = > {});
	}
}

void Room::_handleMediaTransport(rs::WebRtcTransport* transport)
{
	transport->addEventListener("close", [=](Json originator)
	{
		DLOG(INFO) << 
			"Transport 'close' event [originator:" << originator.dump() << "]";
	});
}

void Room::_handleMediaProducer(rs::Producer* producer)
{
	producer->addEventListener("close", [=](Json originator)
	{
		DLOG(INFO) << 
			"Producer 'close' event [originator:" << originator.dump() << "]";
	});

	producer->addEventListener("pause", [=](Json originator)
	{
		DLOG(INFO) << 
			"Producer 'pause' event [originator:" << originator.dump() << "]";
	});

	producer->addEventListener("resume", [=](Json originator)
	{
		DLOG(INFO) << 
			"Producer 'resume' event [originator:" << originator.dump() << "]";
	});
}

void Room::_handleMediaConsumer(rs::Consumer* consumer)
{
	consumer->addEventListener("close", [=](Json originator)
	{
		DLOG(INFO) << 
			"Consumer 'close' event [originator:" << originator.dump() << "]";
	});

	consumer->addEventListener("pause", [=](Json originator)
	{
		DLOG(INFO) << 
			"Consumer 'pause' event [originator:" << originator.dump() << "]";
	});

	consumer->addEventListener("resume", [=](Json originator)
	{
		DLOG(INFO) << 
			"Consumer 'resume' event [originator:" << originator.dump() << "]";
	});

	consumer->addEventListener("effectiveprofilechange", [=](Json profile)
	{
		DLOG(INFO) << 
			"Consumer 'effectiveprofilechange' event [profile:" << profile.dump() << "]";
	});

	// If video, initially make it "low" profile unless this is for the current
	// active speaker.
// 	if (consumer->kind() == "video" && consumer->peer() != this->_currentActiveSpeaker)
// 		consumer->setPreferredProfile("low");
}

void Room::_handleMediasoupClientRequest(protoo::Peer* protooPeer, uint32_t id, Json request)
{
	std::string method = request["method"].get<std::string>();

	DLOG(INFO) <<
		"mediasoup-client request [method:" << method << ", peer:'" << protooPeer->id() << "']";

	if (method == "queryRoom")
	{
		this->_mediaRoom->receiveRequest(request)
		.then([=](Json response)
		{
			protooPeer->Accept(id, response);
		})
		.fail([=](rs::Error error)
		{
			protooPeer->Reject(id, 500, error.ToString());
		});
	}
	else if (method == "join")
	{
		// TODO: Handle appData. Yes?
		std::string peerName = request["peerName"].get<std::string>();

		if (peerName != protooPeer->id())
		{
			protooPeer->Reject(id, 403, "that is not your corresponding mediasoup Peer name");
		}
		else if (protooPeer->mediaPeer())
		{
			protooPeer->Reject(id, 500, "already have a mediasoup Peer");
		}

		this->_mediaRoom->receiveRequest(request)
		.then([=](Json response)
		{
			protooPeer->Accept(id, response);

			// Get the newly created mediasoup Peer.
			rs::Peer* mediaPeer = this->_mediaRoom->getPeerByName(peerName);

			protooPeer->setMediaPeer(mediaPeer);

			this->_handleMediaPeer(protooPeer, mediaPeer);
		})
		.fail([=](rs::Error error)
		{
			protooPeer->Reject(id, 500, error.ToString());
		});
	}
	else
	{
		rs::Peer* mediaPeer = protooPeer->mediaPeer();

		if (!mediaPeer)
		{
			LOG(ERROR) << "cannot handle mediasoup request, no mediasoup Peer [method:"
				<< method << "]";

			protooPeer->Reject(id, 400, "no mediasoup Peer");
		}

		mediaPeer->receiveRequest(request)
		.then([=](Json response)
		{
			protooPeer->Accept(id, response);
		})
		.fail([=](rs::Error error)
		{
			protooPeer->Reject(id, 500, error.ToString().c_str());
		});
	}
}

void Room::_handleMediasoupClientNotification(protoo::Peer* protooPeer, Json notification)
{
	std::string method = notification["method"].get<std::string>();

	DLOG(INFO) <<
		"mediasoup-client notification [method:" << method << ", peer:" << protooPeer->id() << "]";

	// NOTE: mediasoup-client just sends notifications with target "peer",
	// so first of all, get the mediasoup Peer.
	rs::Peer* mediaPeer = protooPeer->mediaPeer();

	if (!mediaPeer)
	{
		LOG(ERROR) <<
			"cannot handle mediasoup notification, no mediasoup Peer [method:" << method << "]";
		return;
	}

	mediaPeer->receiveNotification(notification);
}

void Room::_updateMaxBitrate()
{
	if (this->_mediaRoom->closed())
		return;

	int numPeers = this->_mediaRoom->peers().size();
	uint32_t previousMaxBitrate = this->_maxBitrate;
	uint32_t newMaxBitrate;

	if (numPeers <= 2)
	{
		newMaxBitrate = MAX_BITRATE;
	}
	else
	{
		newMaxBitrate = std::round(MAX_BITRATE / ((numPeers - 1) * BITRATE_FACTOR));

		if (newMaxBitrate < MIN_BITRATE)
			newMaxBitrate = MIN_BITRATE;
	}

	this->_maxBitrate = newMaxBitrate;

	for (auto itPeer : this->_mediaRoom->peers())
	{
		auto peer = itPeer.second;
		for (auto itTransport : peer->transports())
		{
			auto transport = itTransport.second;
			if (transport->direction() == "send")
			{
				transport->setMaxBitrate(newMaxBitrate)
					.fail([=](std::string error)
				{
					LOG(ERROR) << "transport.setMaxBitrate() failed:" << error.c_str();
				});
			}
		}
	}

	DLOG(INFO) <<
		"_updateMaxBitrate() [num peers:" << numPeers
		<< ", before:" << std::round(previousMaxBitrate / 1000)
		<< "kbps, now:" << std::round(newMaxBitrate / 1000) << "kbps]";
}

void Room::Room::spread(std::string method, Json data, std::set<std::string> excluded)
{
	for (auto it : this->_peers)
	{
		if (excluded.count(it.first))
			continue;

		it.second->notify(method, data)
			.fail([]() {});
	}
}

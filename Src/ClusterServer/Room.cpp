#define MS_CLASS "Room"

#include "Room.hpp"
#include "Logger.hpp"
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
	, logger(new rs::Logger("Room"))
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
	//logger->debug("close()");

	this->_closed = true;

	// Close the mediasoup Room.
	if (this->_mediaRoom)
		this->_mediaRoom->close();

	// Emit "close" event.
	this->listener->OnRoomClose(_roomId);
}

void Room::handleConnection(std::string peerName, protoo::WebSocketClient* transport)
{
	logger->info("handleConnection() [peerName:'%s']", peerName.c_str());

	if (this->_peers.count(peerName))
	{
		logger->warn(
			"handleConnection() | there is already a peer with same peerName, \
			closing the previous one [peerName:'%s']",
			peerName.c_str());

		auto protooPeer = this->_peers[peerName];

		protooPeer->close();
	}

	protoo::Peer* protooPeer = new protoo::Peer(peerName, transport, this);

	this->_peers[peerName] = protooPeer;
}

void Room::OnPeerClose(protoo::Peer* peer)
{
	logger->debug("protoo Peer 'close' event [peer:'%s']", peer->id().c_str());

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
// 						logger->info(
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

	logger->debug("protoo 'request' event [method:%s, peer:'%s']",
		method.c_str(), peer->id().c_str());

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
		logger->error("unknown request.method '%s'", method.c_str());

		peer->Reject(id, 400, "unknown request.method " + method);
	}
}

void Room::OnPeerNotify(protoo::Peer* peer, Json& notification)
{

}

void Room::_handleMediaRoom()
{
	logger->debug("_handleMediaRoom()");

	/*
	auto activeSpeakerDetector = this->_mediaRoom->createActiveSpeakerDetector();

	activeSpeakerDetector->on("activespeakerchange", (activePeer) = >
	{
		if (activePeer)
		{
			logger->info("new active speaker [peerName:'%s']", activePeer.name);

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
			logger->info("no active speaker");

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
	mediaPeer->on("notify", [=](Json notification)
	{
		protooPeer->send("mediasoup-notification", notification)
			.fail([]() {});
	});

	mediaPeer->on("newtransport", [=](rs::WebRtcTransport* transport)
	{
		logger->info(
			"mediaPeer 'newtransport' event [id:%d, direction:%s]",
			transport->id(), transport->direction().c_str());

		// Update peers max sending  bitrate.
		if (transport->direction() == "send")
		{
			this->_updateMaxBitrate();

			transport->on("close", [=]()
			{
				this->_updateMaxBitrate();
			});
		}

		this->_handleMediaTransport(transport);
	});

	mediaPeer->on("newproducer", [=](rs::Producer* producer)
	{
		logger->info("mediaPeer 'newproducer' event [id:%d]", producer->id());

		this->_handleMediaProducer(producer);
	});

	mediaPeer->on("newconsumer", [=](rs::Consumer* consumer)
	{
		logger->info("mediaPeer 'newconsumer' event [id:%d]", consumer->id());

		this->_handleMediaConsumer(consumer);
	});

	// Also handle already existing Consumers.
	for (auto itConsumer : mediaPeer->consumers())
	{
		auto consumer = itConsumer.second;

		logger->info("mediaPeer existing 'consumer' [id:%d]", consumer->id());

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
	transport->on("close", [=](Json originator)
	{
		logger->info(
			"Transport 'close' event [originator:%s]", originator.dump().c_str());
	});
}

void Room::_handleMediaProducer(rs::Producer* producer)
{
	producer->on("close", [=](Json originator)
	{
		logger->info(
			"Producer 'close' event [originator:%s]", originator.dump().c_str());
	});

	producer->on("pause", [=](Json originator)
	{
		logger->info(
			"Producer 'pause' event [originator:%s]", originator.dump().c_str());
	});

	producer->on("resume", [=](Json originator)
	{
		logger->info(
			"Producer 'resume' event [originator:%s]", originator.dump().c_str());
	});
}

void Room::_handleMediaConsumer(rs::Consumer* consumer)
{
	consumer->on("close", [=](Json originator)
	{
		logger->info(
			"Consumer 'close' event [originator:%s]", originator.dump().c_str());
	});

	consumer->on("pause", [=](Json originator)
	{
		logger->info(
			"Consumer 'pause' event [originator:%s]", originator.dump().c_str());
	});

	consumer->on("resume", [=](Json originator)
	{
		logger->info(
			"Consumer 'resume' event [originator:%s]", originator.dump().c_str());
	});

	consumer->on("effectiveprofilechange", [=](std::string profile)
	{
		logger->info(
			"Consumer 'effectiveprofilechange' event [profile:%s]", profile.c_str());
	});

	// If video, initially make it "low" profile unless this is for the current
	// active speaker.
// 	if (consumer->kind() == "video" && consumer->peer() != this->_currentActiveSpeaker)
// 		consumer->setPreferredProfile("low");
}

void Room::_handleMediasoupClientRequest(protoo::Peer* protooPeer, uint32_t id, Json request)
{
	std::string method = request["method"].get<std::string>();

	logger->debug(
		"mediasoup-client request [method:%s, peer:'%s']",
		method.c_str(), protooPeer->id().c_str());

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
			logger->error(
				"cannot handle mediasoup request, no mediasoup Peer [method:'%s']",
				method.c_str());

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

	logger->debug(
		"mediasoup-client notification [method:%s, peer:'%s']",
		method.c_str(), protooPeer->id().c_str());

	// NOTE: mediasoup-client just sends notifications with target "peer",
	// so first of all, get the mediasoup Peer.
	rs::Peer* mediaPeer = protooPeer->mediaPeer();

	if (!mediaPeer)
	{
		logger->error(
			"cannot handle mediasoup notification, no mediasoup Peer [method:'%s']",
			method.c_str());

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
					logger->error("transport.setMaxBitrate() failed: %s", error.c_str());
				});
			}
		}
	}

	logger->info(
		"_updateMaxBitrate() [num peers:%d, before:%fkbps, now:%fkbps]",
		numPeers,
		std::round(previousMaxBitrate / 1000),
		std::round(newMaxBitrate / 1000));
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

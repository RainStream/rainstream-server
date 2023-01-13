#ifndef MEDIA_ROOM_HPP
#define MEDIA_ROOM_HPP

#include "common.h"
#include "Peer.hpp"
#include <EnhancedEventEmitter.h>


namespace protoo
{
	class Peer;
	class Request;
	class WebSocketClient;
}

namespace mediasoup {
	class Router;
	class Worker;
	class WebRtcServer;
	class AudioLevelObserver;
}

using namespace mediasoup;

class Bot;

class Room : public protoo::Peer::Listener , public EnhancedEventEmitter
{
public:
	static async_simple::coro::Lazy<Room*> create(Worker* mediasoupWorker, std::string roomId, WebRtcServer* webRtcServer);

	Room(std::string roomId, WebRtcServer* webRtcServer, Router* router, AudioLevelObserver* audioLevelObserver, Bot* bot);
	~Room();

public:
	std::string id();
	void close();
	void logStatus();

	void handleConnection(std::string peerId, bool consume, protoo::WebSocketClient* transport);
	json getRouterRtpCapabilities();
	async_simple::coro::Lazy<json> createBroadcaster(std::string id, std::string displayName, std::string device, json& rtpCapabilities);
	void deleteBroadcaster(std::string broadcasterId);
protected:
	async_simple::coro::Lazy<void> _handleAudioLevelObserver();
	async_simple::coro::Lazy<void> _handleProtooRequest(protoo::Peer* peer, protoo::Request* request);
	std::list<protoo::Peer*> _getJoinedPeers(protoo::Peer* excludePeer = nullptr);

	/**
	 * Creates a mediasoup Consumer for the given mediasoup Producer.
	 *
	 * @async
	 */
	async_simple::coro::Lazy<void> _createConsumer(protoo::Peer* consumerPeer, protoo::Peer* producerPeer, Producer* producer);

	/**
	 * Creates a mediasoup DataConsumer for the given mediasoup DataProducer.
	 *
	 * @async
	 */
	async_simple::coro::Lazy<void> _createDataConsumer(protoo::Peer* dataConsumerPeer,
		protoo::Peer* dataProducerPeer,
		DataProducer* dataProducer);

	/* Methods inherited from protoo::Peer::Listener. */
public:
	void OnPeerClose(protoo::Peer* peer) override;
	void OnPeerRequest(protoo::Peer* peer, protoo::Request* request) override;

private:
	// Room id.
	// @type {String}
	std::string _roomId;

	// Closed flag.
	// @type {Boolean}
	bool _closed = false;

	// @type {Map<String, Object>}
	//this._broadcasters = new Map();

	// mediasoup WebRtcServer instance.
	// @type {mediasoup.WebRtcServer}
	WebRtcServer* _webRtcServer{ nullptr };

	// mediasoup Router instance.
	// @type {mediasoup.Router}
	Router* _mediasoupRouter{ nullptr };

	// mediasoup AudioLevelObserver.
	// @type {mediasoup.AudioLevelObserver}
	AudioLevelObserver* _audioLevelObserver{ nullptr };

	// DataChannel bot.
	// @type {Bot}
	Bot* _bot{ nullptr };

	std::map<std::string, protoo::Peer*> _peers;
};

#endif


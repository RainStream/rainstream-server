#ifndef MEDIA_ROOM_HPP
#define MEDIA_ROOM_HPP

#include "common.hpp"
#include "Peer.hpp"
#include <EnhancedEventEmitter.hpp>


namespace protoo
{
	class Peer;
	class Request;
	class WebSocketClient;
}
class Router;
class Worker;
class WebRtcServer;
class AudioLevelObserver;


class Room : public protoo::Peer::Listener , public EnhancedEventEmitter
{
public:
	static task_t<Room*> create(Worker* mediasoupWorker, std::string roomId, WebRtcServer* webRtcServer);

	Room(std::string roomId, WebRtcServer* webRtcServer, Router* router, AudioLevelObserver* audioLevelObserver);
	~Room();

public:
	std::string id();
	void close();
	void logStatus();

	void handleConnection(std::string peerId, bool consume, protoo::WebSocketClient* transport);

protected:
	task_t<void> _handleAudioLevelObserver();
	task_t<void> _handleProtooRequest(protoo::Peer* peer, protoo::Request* request);
	std::list<protoo::Peer*> _getJoinedPeers(protoo::Peer* excludePeer = nullptr);
	/**
	 * Creates a mediasoup Consumer for the given mediasoup Producer.
	 *
	 * @async
	 */
	task_t<void> _createConsumer(protoo::Peer* consumerPeer, protoo::Peer* producerPeer, Producer* producer);

	/**
	 * Creates a mediasoup DataConsumer for the given mediasoup DataProducer.
	 *
	 * @async
	 */
	task_t<void> _createDataConsumer(protoo::Peer* dataConsumerPeer,
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

	//rs::Peer* _currentActiveSpeaker{ nullptr };

	std::map<std::string, protoo::Peer*> _peers;
};

#endif


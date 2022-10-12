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

class Room : public protoo::Peer::Listener , public EnhancedEventEmitter
{
public:
	class Listener
	{
	public:
		virtual void OnRoomClose(std::string roomId) = 0;
	};
public:

	static task_t<Room*> create(Worker* mediasoupWorker, std::string roomId, WebRtcServer* webRtcServer);

	Room(std::string roomId, WebRtcServer* webRtcServer, Router* router);
	~Room();

public:
	std::string id();
	void close();

	void handleConnection(std::string peerId, bool consume, protoo::WebSocketClient* transport);

protected:
	task_t<void> _handleProtooRequest(protoo::Peer* peer, protoo::Request* request);
	std::list<protoo::Peer*> _getJoinedPeers(protoo::Peer* excludePeer = nullptr);
	/**
	 * Creates a mediasoup Consumer for the given mediasoup Producer.
	 *
	 * @async
	 */
	task_t<void> _createConsumer(protoo::Peer* consumerPeer, protoo::Peer* producerPeer, Producer* producer);

	/* Methods inherited from protoo::Peer::Listener. */
public:
	void OnPeerClose(protoo::Peer* peer) override;
	void OnPeerRequest(protoo::Peer* peer, protoo::Request* request) override;
	void OnPeerNotify(protoo::Peer* peer, json& notification) override;
	
protected:
	void _handleMediaRoom();
// 	void _handleMediaPeer(protoo::Peer* protooPeer, rs::Peer* mediaPeer);
// 	void _handleMediaTransport(rs::WebRtcTransport* transport);
// 	void _handleMediaProducer(rs::Producer* producer);
// 	void _handleMediaConsumer(rs::Consumer* consumer);
// 	void _handleMediasoupClientRequest(protoo::Peer* protooPeer, uint32_t id, json request);
// 	void _handleMediasoupClientNotification(protoo::Peer* protooPeer, json notification);
// 	void _updateMaxBitrate();

	void spread(std::string method, json data, std::set<std::string> excluded = std::set<std::string>());

private:
	// Room id.
	// @type {String}
	std::string _roomId;

	// Closed flag.
	bool _closed = false;

	// @type {Map<String, Object>}
	//this._broadcasters = new Map();

	// mediasoup WebRtcServer instance.
	// @type {mediasoup.WebRtcServer}
	WebRtcServer* _webRtcServer{ nullptr };

	Router* _mediasoupRouter{ nullptr };

	//rs::Peer* _currentActiveSpeaker{ nullptr };
	Listener* listener{ nullptr };

	std::map<std::string, protoo::Peer*> _peers;
};

#endif


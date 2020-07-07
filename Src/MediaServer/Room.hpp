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

class Room : public protoo::Peer::Listener , public EnhancedEventEmitter
{
public:
	class Listener
	{
	public:
		virtual void OnRoomClose(std::string roomId) = 0;
	};
public:

	static std::future<Room*> create(Worker* mediasoupWorker, std::string roomId);

	Room(std::string roomId, Router* router);
	~Room();

public:
	std::string id();
	void close();

	std::future<void> handleProtooRequest(protoo::WebSocketClient* transport, protoo::Request* request);

protected:
	std::future<void> _handleProtooRequest(protoo::Peer* peer, protoo::Request* request);
	std::list<protoo::Peer*> _getJoinedPeers(protoo::Peer* excludePeer = nullptr);
	/**
	 * Creates a mediasoup Consumer for the given mediasoup Producer.
	 *
	 * @async
	 */
	std::future<void> _createConsumer(protoo::Peer* consumerPeer, protoo::Peer* producerPeer, Producer* producer);

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
	std::string _roomId;
// 	rs::Server* _mediaServer{ nullptr };
// 	rs::Room* _mediaRoom{ nullptr };

	Router* _mediasoupRouter{ nullptr };

	//rs::Peer* _currentActiveSpeaker{ nullptr };
	Listener* listener{ nullptr };

	std::map<std::string, protoo::Peer*> _peers;

	// Closed flag.
	bool _closed = false;

	uint32_t _maxBitrate;
};

#endif


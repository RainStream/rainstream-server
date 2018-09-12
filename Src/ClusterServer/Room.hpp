#ifndef MEDIA_ROOM_HPP
#define MEDIA_ROOM_HPP

#include "common.hpp"
#include "protoo/Peer.hpp"

namespace rs
{
	class Server;
	class Room;
	class Logger;
	class Transport;
}

namespace protoo
{
	class Room;
	class Peer;
	class Request;
	class WebSocketTransport;
}

class Room : public protoo::Peer::Listener
{
public:
	class Listener
	{
	public:
		virtual void OnRoomClose(std::string roomId) = 0;
	};
public:
	Room(std::string roomId, Json mediaCodecs, rs::Server* mediaServer, Listener* listener);
	~Room();

public:
	std::string id();
	void close();

	void handleConnection(std::string peerName, protoo::WebSocketTransport* transport);

	/* Methods inherited from protoo::Peer::Listener. */
public:
	void OnPeerClose(protoo::Peer* peer) override;
	void OnPeerRequest(protoo::Peer* peer, protoo::Request* request) override;
	void OnNotification(protoo::Peer* peer, Json notification) override;
	
protected:
	void _handleMediaRoom();
	void _handleMediaPeer(protoo::Peer* protooPeer, rs::Peer* mediaPeer);
	void _handleMediaTransport(rs::WebRtcTransport* transport);
	void _handleMediaProducer(rs::Producer* producer);
	void _handleMediaConsumer(rs::Consumer* consumer);
	void _handleMediasoupClientRequest(protoo::Peer* protooPeer, Json request, AcceptFunc accept, RejectFunc reject);
	void _handleMediasoupClientNotification(protoo::Peer* protooPeer, Json notification);
	void _updateMaxBitrate();


private:
	std::string _roomId;
	rs::Server* _mediaServer{ nullptr };
	rs::Room* _mediaRoom{ nullptr };
	protoo::Room* _protooRoom{ nullptr };
	rs::Peer* _currentActiveSpeaker{ nullptr };
	Listener* listener{ nullptr };

	std::unique_ptr<rs::Logger> logger;

	// Closed flag.
	bool _closed = false;

	uint32_t _maxBitrate;
};

#endif


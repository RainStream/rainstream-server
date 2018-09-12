#ifndef PROTOO_ROOM_HPP
#define PROTOO_ROOM_HPP

#include "common.hpp"
#include "protoo/Peer.hpp"

namespace protoo
{
	class Peer;
	class WebSocketTransport;

	class Room
	{
	public:
		Room();
		~Room();

	public:
		std::string id();
		void close();

		bool hasPeer(std::string peerName);
		Peer* getPeer(std::string peerName);
		Peer* createPeer(std::string peerName, WebSocketTransport* transport, Peer::Listener* listener);
		void spread(std::string method, Json data, std::set<std::string> excluded = std::set<std::string>());

	protected:

	private:
		std::string _roomId;
		std::map<std::string, Peer* > _peers;

		// Closed flag.
		bool _closed = false;
	};
}

#endif


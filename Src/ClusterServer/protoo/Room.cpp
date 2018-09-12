#define MS_CLASS "protoo/Room"

#include "protoo/Room.hpp"
#include "protoo/Peer.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace protoo
{
	Room::Room()
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
	}

	bool Room::hasPeer(std::string peerName)
	{
		return _peers.count(peerName);
	}

	Peer* Room::getPeer(std::string peerName)
	{
		if (hasPeer(peerName))
		{
			return _peers[peerName];
		}
		else
		{
			return nullptr;
		}
	}

	Peer* Room::createPeer(std::string peerName, WebSocketTransport* transport, Peer::Listener* listener)
	{
		Peer* peer = new Peer(peerName, transport, listener);

		_peers[peerName] = peer;

		return peer;
	}

	void Room::spread(std::string method, Json data, std::set<std::string> excluded)
	{
		for (auto it : this->_peers)
		{
			if (excluded.count(it.first))
				continue;

			it.second->notify(method, data)
				.fail([]() {});
		}
	}
}

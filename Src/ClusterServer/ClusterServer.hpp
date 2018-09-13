#ifndef MS_CLUSTER_SERVER_HPP
#define MS_CLUSTER_SERVER_HPP

#include "common.hpp"
#include "Room.hpp"
#include "WebSocketServer.hpp"

namespace protoo
{
	class WebSocketClient;
}

class ClusterServer : public protoo::WebSocketServer::Lisenter, public rs::Server::Listener, public Room::Listener
{
public:
	explicit ClusterServer();
	virtual ~ClusterServer();

protected:
	void OnServerClose() override;
	void OnNewRoom(rs::Room* room) override;
	void OnRoomClose(std::string roomId) override;

	void OnConnectRequest(protoo::WebSocketClient* transport) override;
	void OnConnectClosed(protoo::WebSocketClient* transport) override;

private:

	Json config;

	rs::Server* mediaServer = nullptr;

	protoo::WebSocketServer* webSocketServer = nullptr;

	std::map<std::string, Room*> rooms_;
};

#endif

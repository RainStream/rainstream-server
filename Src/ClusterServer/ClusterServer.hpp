#ifndef MS_CLUSTER_SERVER_HPP
#define MS_CLUSTER_SERVER_HPP

#include <common.hpp>
#include "WebSocketServer.hpp"
#include "AwaitQueue.hpp"


namespace protoo
{
	class WebSocketClient;
}

class Room;
class Worker;

class ClusterServer : public protoo::WebSocketServer::Lisenter
{
public:
	explicit ClusterServer();
	virtual ~ClusterServer();

protected:
	void OnRoomClose(std::string roomId);

	void OnConnectRequest(protoo::WebSocketClient* transport) override;
	void OnConnectClosed(protoo::WebSocketClient* transport) override;

protected:
	void connectionrequest(protoo::WebSocketClient* transport);

	void runMediasoupWorkers();
	/**
	 * Get next mediasoup Worker.
	 */
	Worker* getMediasoupWorker();

	std::future<Room*> getOrCreateRoom(std::string roomId);

private:

	json config;

	protoo::WebSocketServer* _webSocketServer = nullptr;

	std::vector<Worker*> _mediasoupWorkers;

	std::map<std::string, Room*> _rooms;

	AwaitQueue<void> _queue;
};

#endif

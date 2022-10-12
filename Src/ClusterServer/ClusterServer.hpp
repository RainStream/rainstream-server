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
class WebRtcServer;

class ClusterServer : public protoo::WebSocketServer::Lisenter
{
public:
	explicit ClusterServer();
	virtual ~ClusterServer();

protected:
	void OnRoomClose(std::string roomId);

	void OnConnectRequest(std::string requestUrl, protoo::FnAccept accept, protoo::FnReject reject) override;
	void OnConnectClosed(protoo::WebSocketClient* transport) override;

protected:
	task_t<void> runMediasoupWorkers();
	/**
	 * Get next mediasoup Worker.
	 */
	Worker* getMediasoupWorker();

	task_t<Room*> getOrCreateRoom(std::string roomId);

private:
	protoo::WebSocketServer* _webSocketServer = nullptr;

	std::vector<Worker*> _mediasoupWorkers;

	std::map<Worker*, WebRtcServer*> _workerWebRtcServers;

	std::map<std::string, Room*> _rooms;

	AwaitQueue<void> _queue;
};

#endif

#ifndef MS_CLUSTER_SERVER_HPP
#define MS_CLUSTER_SERVER_HPP

#include <common.h>
#include "WebSocketServer.hpp"
#include "AwaitQueue.hpp"


namespace protoo
{
	class WebSocketClient;
}

namespace mediasoup {
	
	class Worker;
	class WebRtcServer;
}

using namespace mediasoup;

class Room;

class ClusterServer : public protoo::WebSocketServer::Lisenter
{
public:
	explicit ClusterServer();
	virtual ~ClusterServer();

protected:
	void OnConnectRequest(std::string requestUrl, const protoo::FnAccept& accept, const  protoo::FnReject& reject) override;
	void OnConnectClosed(protoo::WebSocketClient* transport) override;

protected:
	std::future<void> runMediasoupWorkers();

	std::future<void> runHttpsServer();

	Worker* getMediasoupWorker();

	std::future<Room*> getOrCreateRoom(std::string roomId);

private:
	protoo::WebSocketServer* _webSocketServer;

	std::vector<Worker*> _mediasoupWorkers;

	std::map<Worker*, WebRtcServer*> _workerWebRtcServers;

	std::map<std::string, Room*> _rooms;

	//AwaitQueue<void> _queue;
};

#endif

#ifndef MS_CLUSTER_SERVER_HPP
#define MS_CLUSTER_SERVER_HPP

#include <common.hpp>
#include "WebSocketServer.hpp"

namespace protoo
{
	class WebSocketClient;
}

class Room;
class Worker;

class MediaServer : public protoo::WebSocketServer::Lisenter
{
public:
	explicit MediaServer();
	virtual ~MediaServer();

protected:
	void OnRoomClose(std::string roomId);

	void OnConnectRequest(protoo::WebSocketClient* transport) override;
	void OnConnectClosed(protoo::WebSocketClient* transport) override;

protected:
	std::future<void> connectionrequest(protoo::WebSocketClient* transport);

	void runMediasoupWorkers();
	/**
	 * Get next mediasoup Worker.
	 */
	Worker* getMediasoupWorker();

	std::future<Room*> getOrCreateRoom(std::string roomId);

private:

	json config;

	protoo::WebSocketServer* webSocketServer = nullptr;

	std::vector<Worker*> mediasoupWorkers;
	std::map<std::string, Room*> rooms_;
};

#endif

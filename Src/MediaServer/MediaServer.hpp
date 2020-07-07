#ifndef MS_CLUSTER_SERVER_HPP
#define MS_CLUSTER_SERVER_HPP

#include <common.hpp>
#include "WebSocketClient.hpp"
#include "WebSocketServer.hpp"

namespace protoo
{
	class Request;
	class WebSocketClient;
}

namespace uS
{
	struct Timer;
}

class Room;
class Worker;

class MediaServer : public protoo::WebSocketServer::Lisenter, public protoo::WebSocketClient::Listener
{
public:
	explicit MediaServer();
	virtual ~MediaServer();

protected:
	void OnRoomClose(std::string roomId);

	virtual void OnConnected(protoo::WebSocketClient* transport) override;

	virtual std::future<void> OnRequest(protoo::WebSocketClient* transport, protoo::Request* request) override;
	virtual void OnClosed(int code, const std::string& message) override;

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
	
	uS::Timer* timer;

};

#endif

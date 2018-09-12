#ifndef MS_CLUSTER_SERVER_HPP
#define MS_CLUSTER_SERVER_HPP

#include "common.hpp"
#include "Room.hpp"
#include "protoo/WebSocketServer.hpp"

namespace protoo
{
	class WebSocketTransport;
}

class ClusterServer : public protoo::WebSocketServer::Lisenter, public rs::Server::Listener, public Room::Listener
{
public:
	explicit ClusterServer();
	virtual ~ClusterServer();

	/**
	* Create a Server instance.
	*
	* @param {object} [options]
	* @param {number} [options.numWorkers=HOST_NUM_CORES] - Number of child workers.
	* @param {string} [options.logLevel='debug'] - Log level. Valid values are
	* 'debug', 'warn', 'error'.
	* @param {array} [options.logTags] - Log tags.
	* @param {string|boolean} [options.rtcIPv4=true] - IPv4 for RTC. Valid
	* values are a IPv4, `true` (auto-detect) and `false` (disabled).
	* @param {string|boolean} [options.rtcIPv6=true] - IPv6 for RTC. Valid
	* values are a IPv6, `true` (auto-detect) and `false` (disabled).
	* @param {string} [options.rtcAnnouncedIPv4] - Announced IPv4 for RTC. Valid
	* value is a IPv4.
	* @param {string} [options.rtcAnnouncedIPv6] - Announced IPv6 for RTC. Valid
	* value is a IPv6.
	* @param {number} [options.rtcMinPort=10000] - Minimun RTC port.
	* @param {number} [options.rtcMaxPort=59999] - Maximum RTC port.
	*
	* @return {Server}
	*/



protected:
	void OnServerClose() override;
	void OnNewRoom(rs::Room* room) override;

	void OnRoomClose(std::string roomId) override;

	void connectionrequest(std::string url, protoo::WebSocketTransport* transport) override;

private:

	Json config;

	rs::Server* mediaServer = nullptr;

	protoo::WebSocketServer* webSocketServer = nullptr;

	std::map<std::string, Room*> rooms_;
};

#endif

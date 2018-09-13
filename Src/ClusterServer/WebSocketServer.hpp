#ifndef PROTOO_WEBSOCKET_SERVER_HPP
#define PROTOO_WEBSOCKET_SERVER_HPP

#include "common.hpp"

namespace uWS {
	struct Hub;
}

namespace protoo
{
	class WebSocketClient;

	class WebSocketServer
	{
	public:
		class Lisenter
		{
		public:
			virtual void OnConnectRequest(WebSocketClient* transport) = 0;
			virtual void OnConnectClosed(WebSocketClient* transport) = 0;
		};
	public:
		explicit WebSocketServer(Json tls, Lisenter* lisenter);
		WebSocketServer& operator=(const WebSocketServer&) = delete;
		WebSocketServer(const WebSocketServer&) = delete;

	protected:
		virtual	~WebSocketServer();

	public:
		bool Setup(const char *host, uint16_t port);

	private:
		Json tls;
		Lisenter * lisenter{ nullptr };
		// Closed flag.
		bool _closed = false;
		uWS::Hub *hub = nullptr;
	};
}

#endif


#ifndef PROTOO_WEBSOCKET_SERVER_HPP
#define PROTOO_WEBSOCKET_SERVER_HPP

#include "common.hpp"

namespace protoo
{
	class WebSocketClient;

	class WebSocketServer
	{
	public:
		class Lisenter
		{
		public:
			virtual void OnConnected(WebSocketClient* transport) = 0;
		};
	public:
		explicit WebSocketServer(json tls, Lisenter* lisenter);
		WebSocketServer& operator=(const WebSocketServer&) = delete;
		WebSocketServer(const WebSocketServer&) = delete;

	protected:
		virtual	~WebSocketServer();

	public:
		bool Connect(std::string url);

	private:
		json tls;
		Lisenter * lisenter{ nullptr };
		// Closed flag.
		bool _closed = false;
	};
}

#endif


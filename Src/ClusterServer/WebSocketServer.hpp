#ifndef WEBSOCKET_SERVER_HPP
#define WEBSOCKET_SERVER_HPP

#include "common.hpp"
#include <uwebsockets/App.h>
#include <uwebsockets/WebSocket.h>

namespace protoo {

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
	explicit WebSocketServer(json tls, Lisenter* lisenter);
	WebSocketServer& operator=(const WebSocketServer&) = delete;
	WebSocketServer(const WebSocketServer&) = delete;

protected:
	virtual	~WebSocketServer();

public:
	bool Setup(const char* host, uint16_t port);

protected:


private:
	json _tls;
	Lisenter* _lisenter{ nullptr };
	// Closed flag.
	bool _closed = false;
	//uWS::Hub *hub = nullptr;

	uWS::SSLApp* _app{ nullptr };
};
}
#endif


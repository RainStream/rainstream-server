#ifndef WEBSOCKET_SERVER_HPP
#define WEBSOCKET_SERVER_HPP

#include "common.h"
#include "errors.h"
#include <uwebsockets/App.h>
#include <uwebsockets/WebSocket.h>

namespace protoo {

class WebSocketClient;

using FnAccept = std::function<WebSocketClient*(void)>;
using FnReject = std::function<void(Error)>;

class WebSocketServer
{
public:
	class Lisenter
	{
	public:
		virtual void OnConnectRequest(std::string requestUrl, const FnAccept& accept, const FnReject& reject) = 0;
		virtual void OnConnectClosed(WebSocketClient* transport) = 0;
	};
public:
	explicit WebSocketServer(json tls, Lisenter* lisenter);
	WebSocketServer& operator=(const WebSocketServer&) = delete;
	WebSocketServer(const WebSocketServer&) = delete;

	void get(std::string pattern, uWS::MoveOnlyFunction<void(uWS::HttpResponse<true>*, uWS::HttpRequest*)>&& handler);

	void post(std::string pattern, uWS::MoveOnlyFunction<void(uWS::HttpResponse<true>*, uWS::HttpRequest*)>&& handler);

	void del(std::string pattern, uWS::MoveOnlyFunction<void(uWS::HttpResponse<true>*, uWS::HttpRequest*)>&& handler);

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


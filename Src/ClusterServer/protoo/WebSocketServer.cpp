#define MS_CLASS "protoo/WebSocketServer"

#include "protoo/WebSocketServer.hpp"
#include "protoo/WebSocketTransport.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <iostream>
#include <fstream>

#include <uWS/uWS.h>

#define SEC_WEBSOCKET_PROTOCOL "protoo"

namespace protoo
{
// 	inline static void onConnection(uWS::WebSocket<uWS::SERVER> *ws, uWS::HttpRequest req)
// 	{
// 		//static_cast<WebSocketServer*>(handle->data)->OnUvReadAlloc(suggestedSize, buf);
// 	}


	WebSocketServer::WebSocketServer(Json tls, Lisenter* lisenter)
		: lisenter(lisenter)
		, tls(tls)
	{
		//here must use default loop
		hub = new uWS::Hub(0, true);

		hub->onConnection([=](uWS::WebSocket<uWS::SERVER> *ws, uWS::HttpRequest req) 
		{
			if (req.getHeader("sec-websocket-protocol").toString() != SEC_WEBSOCKET_PROTOCOL) {

				MS_INFO(YELLOW, "_onRequest() | invalid/missing Sec-WebSocket-Protocol\n");

				ws->close(403, "Invalid/missing Sec-WebSocket-Protocol");

				return;
			}

			WebSocketTransport* transport = new WebSocketTransport;
			transport->setUserData(ws);
			ws->setUserData(transport);

			std::string url = req.getUrl().toString();

			if (lisenter)
			{
				lisenter->connectionrequest(url, transport);
			}
		});

		hub->onMessage([=](uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode opCode)
		{
			WebSocketTransport* transport = (WebSocketTransport*)ws->getUserData();
			if (transport)
			{
				transport->onMessage(std::string(message, length));
			}
		});

		hub->onDisconnection([=](uWS::WebSocket<uWS::SERVER> *ws, int code, char *message, size_t length)
		{
			WebSocketTransport* transport = (WebSocketTransport*)ws->getUserData();
			if (transport)
			{
				transport->onDisconnection(code, std::string(message, length));
				delete transport;
			}

			ws->setUserData(nullptr);
		});
	}
	
	WebSocketServer::~WebSocketServer()
	{
		if (hub)
		{
			delete hub;
			hub = nullptr;
		}
	}

	bool WebSocketServer::Setup(const char *host, uint16_t port)
	{
		std::string cert, key;

		if (tls.is_object())
		{
			cert = tls.value("cert","");
			key = tls.value("key", "");
		}

		uS::TLS::Context c = uS::TLS::createContext(tls["cert"], tls["key"]);
		return hub->listen(host, port, c);
	}
}

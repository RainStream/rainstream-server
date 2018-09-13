#define MS_CLASS "WebSocketServer"

#include "WebSocketServer.hpp"
#include "WebSocketClient.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <iostream>
#include <fstream>

#include <uWS/uWS.h>

#define SEC_WEBSOCKET_PROTOCOL "protoo"

namespace protoo
{
	WebSocketServer::WebSocketServer(Json tls, Lisenter* lisenter)
		: lisenter(lisenter)
		, tls(tls)
	{
		//here must use default loop
		hub = new uWS::Hub(0, true);

		hub->onConnection([=](uWS::WebSocket<uWS::SERVER> *ws, uWS::HttpRequest req) 
		{
			if (req.getHeader("sec-websocket-protocol").toString() != SEC_WEBSOCKET_PROTOCOL) {

				ws->close(403, "Invalid/missing Sec-WebSocket-Protocol");

				return;
			}

			std::string url = req.getUrl().toString();

			WebSocketClient* transport = new WebSocketClient(url);
			transport->setUserData(ws);
			ws->setUserData(transport);

			if (lisenter)
			{
				lisenter->OnConnectRequest(transport);
			}
		});

		hub->onMessage([=](uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode opCode)
		{
			WebSocketClient* transport = (WebSocketClient*)ws->getUserData();
			if (transport)
			{
				transport->onMessage(std::string(message, length));
			}
		});

		hub->onDisconnection([=](uWS::WebSocket<uWS::SERVER> *ws, int code, char *message, size_t length)
		{
			WebSocketClient* transport = (WebSocketClient*)ws->getUserData();
			if (transport)
			{
				if (lisenter)
				{
					lisenter->OnConnectClosed(transport);
				}

				transport->onClosed(code, std::string(message, length));
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

		uS::TLS::Context c = uS::TLS::createContext(cert, key);
		return hub->listen(host, port, c);
	}
}

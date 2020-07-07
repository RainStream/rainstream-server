#define MSC_CLASS "WebSocketServer"

#include "WebSocketServer.hpp"
#include "WebSocketClient.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <iostream>
#include <fstream>
#include "DepLibUV.hpp"
#include <uWS.h>

#define SEC_WEBSOCKET_PROTOCOL "secret-media"

namespace protoo
{
	WebSocketServer::WebSocketServer(json tls, Lisenter* lisenter)
		: lisenter(lisenter)
		, tls(tls)
	{
		//here must use default loop
		auto hub = DepLibUV::GetHub();

		hub->onError([](void *user) {
			std::cout << "FAILURE: " << user << " should not emit error!" << std::endl;
		});

		hub->onConnection([=](uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req)
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
				lisenter->OnConnected(transport);
			}
		});

		hub->onMessage([=](uWS::WebSocket<uWS::CLIENT> *ws, char *message, size_t length, uWS::OpCode opCode)
		{
			std::string msg(message, length);
			WebSocketClient* transport = (WebSocketClient*)ws->getUserData();
			if (transport)
			{
				transport->onMessage(msg);
			}
		});

		hub->onDisconnection([=](uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, size_t length)
		{
			WebSocketClient* transport = (WebSocketClient*)ws->getUserData();
			if (transport)
			{
				transport->OnClosed(code, std::string(message, length));
				delete transport;
			}

			ws->setUserData(nullptr);
		});
	}
	
	WebSocketServer::~WebSocketServer()
	{

	}

	bool WebSocketServer::Connect(std::string url)
	{
		auto hub = DepLibUV::GetHub();

		std::map<std::string, std::string> extraHeaders;
		extraHeaders.insert(std::make_pair("sec-websocket-protocol", SEC_WEBSOCKET_PROTOCOL));

		hub->connect(url, (void*)this, extraHeaders);

		return true;
	}
}

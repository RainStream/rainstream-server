#define MSC_CLASS "WebSocketServer"

#include "WebSocketServer.hpp"
#include "WebSocketClient.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "errors.hpp"
#include <iostream>
#include <fstream>
#include "DepLibUV.hpp"
#include <uWS.h>

#define RECONNECT_TIMER 10
#define SEC_WEBSOCKET_PROTOCOL "secret-media"

namespace protoo
{
	WebSocketServer::WebSocketServer(Lisenter* lisenter)
		: lisenter(lisenter)
	{
		//here must use default loop
		auto hub = DepLibUV::GetHub();

		timer_ = new uS::Timer(hub->getLoop());
		timer_->setData(this);

		hub->onError([=](void *user) {
			WebSocketServer * pThis = static_cast<WebSocketServer*>(user);
			pThis->onDisConnected();
		});

		hub->onConnection([=](uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req)
		{
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

			this->onDisConnected();
		});
	}
	
	WebSocketServer::~WebSocketServer()
	{
		timer_->close();
	}

	bool WebSocketServer::Connect(std::string url)
	{
		if (url.empty())
		{
			return false;
		}

		url_ = url;

		doConnect();

		return true;
	}

	void  WebSocketServer::doConnect()
	{
		timer_->stop();

		auto hub = DepLibUV::GetHub();

		std::map<std::string, std::string> extraHeaders;
		extraHeaders.insert(std::make_pair("sec-websocket-protocol", SEC_WEBSOCKET_PROTOCOL));

		hub->connect(url_, (void*)this, extraHeaders);
	}

	void  WebSocketServer::onDisConnected()
	{
		MSC_ERROR("connect to websocket server error! Try to reconnect after %d seconds", RECONNECT_TIMER);

		timer_->start([](uS::Timer *timer)
		{
			WebSocketServer *pThis = static_cast<WebSocketServer*>(timer->getData());
			pThis->doConnect();

		}, RECONNECT_TIMER, RECONNECT_TIMER);
	}

}

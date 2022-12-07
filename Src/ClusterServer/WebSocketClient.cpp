#define MSC_CLASS "WebSocketClient"

#include "WebSocketClient.hpp"
#include "Logger.h"
#include "Utils.h"
#include <errors.h>
#include <uwebsockets/App.h>
#include <uwebsockets/WebSocket.h>

namespace protoo {

WebSocketClient::WebSocketClient(std::string url)
	: _url(url)
{
}

WebSocketClient::~WebSocketClient()
{
}

void WebSocketClient::close(int code, std::string message)
{
	if (_closed)
	{
		return;
	}

	_closed = true;

	if (userData)
	{
		auto ws = static_cast <uWS::WebSocket<true, true, PeerSocketData>*>(userData);
		ws->end(code, message.c_str());
	}
}

void WebSocketClient::send(const json& data)
{
	if (this->_closed)
		MSC_THROW_ERROR("transport closed");

	try
	{
		auto ws = static_cast<uWS::WebSocket<true, true, PeerSocketData>*>(userData);
		ws->send(data.dump(), uWS::OpCode::TEXT);
	}
	catch (std::exception& error)
	{
		MSC_ERROR("send() failed:%s", error.what());
		throw error;
	}
}

bool WebSocketClient::closed()
{
	return this->_closed;
}

void WebSocketClient::setListener(Listener* listener)
{
	_listener = listener;
}

std::string WebSocketClient::url() const
{
	return _url;
}

std::string WebSocketClient::addresss() const
{
	return _address;
}

void WebSocketClient::setUserData(void* userData)
{
	this->userData = userData;

	auto ws = static_cast <uWS::WebSocket<true, true, PeerSocketData>*>(userData);
	_address = std::string(ws->getRemoteAddress());
}

void WebSocketClient::onMessage(const std::string& message)
{
	if (_listener)
	{
		_listener->onMessage(message);
	}
}

void WebSocketClient::onClosed(int code, const std::string& message)
{
	if (this->_listener)
	{
		_listener->onClosed(code, message);
	}
}

}
